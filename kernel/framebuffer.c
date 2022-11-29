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

struct {
  struct spinlock lock;
} fb;

//
// user write()s to the framebuffer go here.
//
int
framebufferwrite(int user_src, uint64 src, uint off, int n)
{
  return either_copyin((void *)0x80600000+off, user_src, src, n);
}

//
// user read()s from the framebuffer go here.
//
int
framebufferread(int user_dst, uint64 dst, uint off, int n)
{
  return either_copyout(user_dst, dst, (void *)0x80600000+off, n);
}

void
framebufferinit(void)
{
  initlock(&fb.lock, "fb");

  // connect read and write system calls
  devsw[FRAMEBUFFER].read = framebufferread;
  devsw[FRAMEBUFFER].write = framebufferwrite;
}
