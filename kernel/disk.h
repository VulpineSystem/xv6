//
// disk device definitions.
// custom interface for VulpineSystem.
//

// disk mmio control registers, mapped starting at 0x10001000.
#define DISK_MMIO_MAGIC_VALUE 0x000 // 0x666F7864
#define DISK_MMIO_VERSION 0x004 // version; should be 1
#define DISK_MMIO_NOTIFY 0x008 // -1 = idle
#define DISK_MMIO_DIRECTION 0x00C // 0 = read, 1 = write
#define DISK_MMIO_BUFFER_ADDR_HIGH 0x010
#define DISK_MMIO_BUFFER_ADDR_LOW 0x014
#define DISK_MMIO_BUFFER_LEN_HIGH 0x018
#define DISK_MMIO_BUFFER_LEN_LOW 0x01C
#define DISK_MMIO_SECTOR 0x020

// this many disk descriptors.
// must be a power of two.
#define NUM 8

// a single descriptor, from the spec.
struct virtq_desc {
  uint64 addr;
  uint32 len;
  uint16 flags;
  uint16 next;
};
#define VRING_DESC_F_NEXT  1 // chained with another descriptor

// the format of the first descriptor in a disk request.
// to be followed by two more descriptors containing
// the block, and a one-byte status.
struct virtio_blk_req {
  uint32 type; // VIRTIO_BLK_T_IN or ..._OUT
  uint32 reserved;
  uint64 sector;
};
