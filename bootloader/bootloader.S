#define DISK_MMIO_BASE 0x10001000
#define DISK_MMIO_NOTIFY 0x008 // -1 = idle
#define DISK_MMIO_DIRECTION 0x00C // 0 = read, 1 = write
#define DISK_MMIO_BUFFER_ADDR_HIGH 0x010
#define DISK_MMIO_BUFFER_ADDR_LOW 0x014
#define DISK_MMIO_BUFFER_LEN_HIGH 0x018
#define DISK_MMIO_BUFFER_LEN_LOW 0x01C
#define DISK_MMIO_SECTOR 0x020
#define DISK_MMIO_DONE 0x024

#define FRAMEBUFFER 0x80600000
#define FRAMEBUFFER_SIZE 640*480*4

#define BLOCK_SIZE 1024
#define BLOCK_TO_SECTOR(b) b * (BLOCK_SIZE / 512)

#define FS_MAGIC 0x10203040

.section .text
.global _entry
_entry:
        # set up a stack
        la sp, stack0
        li a0, 1024*4
        csrr a1, mhartid
        addi a1, a1, 1
        mul a0, a0, a1
        add sp, sp, a0

        # enable the bare minimum of interrupts so we can access the disk device
        csrr a0, mstatus
        ori a0, a0, 8
        csrw mstatus, a0

        # access the disk mmio registers by offsetting a7
        li a7, DISK_MMIO_BASE

        # set buffer address and length
        la a0, disk_buffer
        sw a0, DISK_MMIO_BUFFER_ADDR_LOW(a7)
        li a0, 0
        sw a0, DISK_MMIO_BUFFER_ADDR_HIGH(a7)
        li a0, BLOCK_SIZE
        sw a0, DISK_MMIO_BUFFER_LEN_LOW(a7)
        li a0, 0
        sw a0, DISK_MMIO_BUFFER_LEN_HIGH(a7)

        # read superblock
        li a0, BLOCK_TO_SECTOR(1)
        call read_sector
        la a0, disk_buffer
        la a1, superblock
        li a2, 8
sb_loop:
        lw a3, 0(a0)
        sw a3, 0(a1)
        addi a0, a0, 4
        addi a1, a1, 4
        addi a2, a2, -1
        bne zero, a2, sb_loop

        # check filesystem magic value
        la a0, superblock.magic
        lw a0, 0(a0)
        li a1, FS_MAGIC
        bne a0, a1, error

spin:
        j spin

# read a sector into the disk buffer
# inputs:
# a0: sector number
# outputs:
# none
read_sector:
        sw a0, DISK_MMIO_SECTOR(a7)
        sw zero, DISK_MMIO_DIRECTION(a7)
        sw zero, DISK_MMIO_NOTIFY(a7)
.wait:
        # wait for done to equal zero
        lw a0, DISK_MMIO_DONE(a7)
        bne zero, a0, .wait
        # done is zero, set it back to -1
        li a0, -1
        sw a0, DISK_MMIO_DONE(a7)
        ret

error:
        li a0, FRAMEBUFFER
        li a1, 0xFF000080
        li a2, FRAMEBUFFER_SIZE
.loop:
        sw a1, 0(a0)
        addi a0, a0, 4
        addi a2, a2, -4
        bne zero, a2, .loop
        j spin

.section .bss
.p2align 2
stack0:
        .space 4096
.p2align 2
disk_buffer:
        .space BLOCK_SIZE
.p2align 2
superblock:
superblock.magic:      .space 4
superblock.size:       .space 4
superblock.nblocks:    .space 4
superblock.ninodes:    .space 4
superblock.nlog:       .space 4
superblock.logstart:   .space 4
superblock.inodestart: .space 4
superblock.bmapstart:  .space 4
superblock_size = . - superblock
