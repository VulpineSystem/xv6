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

#define BACKGROUND_COLOR 0xff2e1e1e

struct {
  struct spinlock lock;
} fb;

//
// user write()s to the framebuffer go here.
//
int
framebufferwrite(int user_src, uint64 src, uint off, int n)
{
  return either_copyin((void *)0x80600000+(off%FRAMEBUFFER0_SIZE), user_src, src, n);
}

//
// user read()s from the framebuffer go here.
//
int
framebufferread(int user_dst, uint64 dst, uint off, int n)
{
  return either_copyout(user_dst, dst, (void *)0x80600000+(off%FRAMEBUFFER0_SIZE), n);
}

void
framebufferinit(void)
{
  initlock(&fb.lock, "fb");

  // fill framebuffer with background color
  uint32 *framebuffer = (void *)0x80600000;
  for (int i = 640*480; i > 0; i--) {
    framebuffer[i] = BACKGROUND_COLOR;
  }

  // connect read and write system calls
  devsw[FRAMEBUFFER].read = framebufferread;
  devsw[FRAMEBUFFER].write = framebufferwrite;
}
