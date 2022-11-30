//
// driver for VulpineSystem's disk device.
//

#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "fs.h"
#include "buf.h"
#include "disk.h"

// the address of mmio register r.
#define R(r) ((volatile uint32 *)(DISK0 + (r)))

static struct disk {
  struct spinlock disk_lock;
} disk;

void
disk_init(void)
{
  initlock(&disk.disk_lock, "disk");

  if(*R(DISK_MMIO_MAGIC_VALUE) != 0x666F7864 ||
     *R(DISK_MMIO_VERSION) != 1){
    panic("could not find disk device");
  }

  // plic.c and trap.c arrange for interrupts from DISK0_IRQ.
}

void
disk_rw(struct buf *b, int write)
{
  uint64 sector = b->blockno * (BSIZE / 512);

  acquire(&disk.disk_lock);

  *R(DISK_MMIO_BUFFER_ADDR_HIGH) = (uint64) &b->data >> 32;
  *R(DISK_MMIO_BUFFER_ADDR_LOW) = (uint64) &b->data & 0xFFFFFFFF;

  *R(DISK_MMIO_BUFFER_LEN_HIGH) = 0;
  *R(DISK_MMIO_BUFFER_LEN_LOW) = BSIZE;

  *R(DISK_MMIO_SECTOR) = sector;

  if(write)
    *R(DISK_MMIO_DIRECTION) = 1; // device reads b->data
  else
    *R(DISK_MMIO_DIRECTION) = 0; // device writes b->data

  *R(DISK_MMIO_NOTIFY) = 0;

  while(*R(DISK_MMIO_DONE) != 0);
  *R(DISK_MMIO_DONE) = -1;

  release(&disk.disk_lock);
}

void
disk_intr()
{
  // nothing
}
