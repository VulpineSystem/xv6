//
// Console input and output, to the framebuffer.
// Reads are line at a time.
// Implements special input characters:
//   newline -- end of line
//   control-h -- backspace
//   control-u -- kill line
//   control-d -- end of file
//   control-p -- print process list
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

#define C(x)  ((x)-'@')  // Control-x

//
// send one character to the console.
// called by printf(), and to echo input characters,
// but not from write().
//
void
consputc(int c)
{
  if(c == '\b'){
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

  // input
#define INPUT_BUF_SIZE 128
  char buf[INPUT_BUF_SIZE];
  uint r; // Read index
  uint w; // Write index
  uint e; // Edit index
  int do_read;
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
// this is a mishmash of the previous console
// driver code, and new driver code. try to not
// lose too many braincells by looking at this.
//
int
consoleread(int user_dst, uint64 dst, uint off, int n)
{
  uint target;
  int c = 0;
  char cbuf;

  target = n;
  acquire(&cons.lock);

  while(cons.do_read == 0){
    if(killed(myproc())){
      release(&cons.lock);
      return -1;
    }
    c = scancode_to_ascii(read_keyboard());
    switch(c){
    case C('P'):  // Print process list.
      procdump();
      break;
    case C('U'):  // Kill line.
      while(cons.e != cons.w &&
            cons.buf[(cons.e-1) % INPUT_BUF_SIZE] != '\n'){
        cons.e--;
        consputc('\b');
      }
      break;
    case C('H'): // Backspace
    case '\x7f': // Delete key
      if(cons.e != cons.w){
        cons.e--;
        consputc('\b');
      }
      break;
    default:
      if(c != 0 && cons.e-cons.r < INPUT_BUF_SIZE){
        // echo back to the user.
        consputc(c);

        // store for consumption.
        cons.buf[cons.e++ % INPUT_BUF_SIZE] = c;

        if(c == '\n' || c == C('D') || cons.e-cons.r == INPUT_BUF_SIZE){
          cons.w = cons.e;
          cons.do_read = 1;
        }
      }
      break;
    }
  }

  while(n > 0){
    c = cons.buf[cons.r++ % INPUT_BUF_SIZE];

    if(c == C('D')){  // end-of-file
      if(n < target){
        // Save ^D for next time, to make sure
        // caller gets a 0-byte result.
        cons.r--;
      }
      break;
    }

    // copy the input byte to the user-space buffer.
    cbuf = c;
    if(either_copyout(user_dst, dst, &cbuf, 1) == -1)
      break;

    dst++;
    --n;

    if(c == '\n'){
      // a whole line has arrived, return to
      // the user-level read().
      cons.do_read = 0;
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
  // nothing
}

void
consoleinit(void)
{
  initlock(&cons.lock, "cons");

  uartinit();

  cons.do_read = 0;

  // connect read and write system calls
  // to consoleread and consolewrite.
  devsw[CONSOLE].read = consoleread;
  devsw[CONSOLE].write = consolewrite;
}
