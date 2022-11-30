//
// Console input and output, to the framebuffer.
// Reads are line at a time.
// Implements special input characters:
//   newline -- end of line
//   control-p on uart -- print process list
//

#include <stdarg.h>

#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "fs.h"
#include "file.h"
#include "memlayout.h"
#include "riscv.h"
#include "defs.h"
#include "proc.h"

#define BACKSPACE 0x100
#define C(x)  ((x)-'@')  // Control-x

//
// send one character to the console.
// called by printf(), and to echo input characters,
// but not from write().
//
void
consputc(int c)
{
  if(c == BACKSPACE){
    // if the user typed backspace, overwrite with a space.
    fb_console_print_character('\b');
    fb_console_print_character(' ');
    fb_console_print_character('\b');
  } else {
    fb_console_print_character(c);
  }
  fb_console_redraw_line();
}

struct {
  struct spinlock lock;
} cons;

//
// user write()s to the console go here.
//
int
consolewrite(int user_src, uint64 src, uint off, int n)
{
  int i;

  for(i = 0; i < n; i++){
    char c;
    if(either_copyin(&c, user_src, src+i, 1) == -1)
      break;
    fb_console_print_character(c);
    fb_console_redraw_line();
  }

  return i;
}

//
// user read()s from the console go here.
// copy (up to) a whole input line to dst.
// user_dist indicates whether dst is a user
// or kernel address.
//
int
consoleread(int user_dst, uint64 dst, uint off, int n)
{
  uint target;
  int c = 0;
  char cbuf;

  target = n;
  acquire(&cons.lock);
  while(n > 0){
    while(c == 0){
      if(killed(myproc())){
        release(&cons.lock);
        return -1;
      }
      c = scancode_to_ascii(read_keyboard());
    }

    // copy the input byte to the user-space buffer.
    cbuf = c;
    if(either_copyout(user_dst, dst, &cbuf, 1) == -1)
      break;

    dst++;
    --n;

    consputc(c);

    if(c == '\n'){
      // a whole line has arrived, return to
      // the user-level read().
      break;
    }
  }
  release(&cons.lock);

  return target - n;
}

//
// uartintr() calls this for input character.
//
void
consoleintr(int c)
{
  acquire(&cons.lock);

  switch(c){
  case C('P'):  // Print process list.
    procdump();
    break;
  }

  release(&cons.lock);
}

void
consoleinit(void)
{
  initlock(&cons.lock, "cons");

  uartinit();

  // connect read and write system calls
  // to consoleread and consolewrite.
  devsw[CONSOLE].read = consoleread;
  devsw[CONSOLE].write = consolewrite;
}
