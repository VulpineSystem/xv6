#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <assert.h>

#define stat xv6_stat  // avoid clash with host struct stat
#include "kernel/types.h"
#include "kernel/fs.h"
#include "kernel/stat.h"
#include "kernel/param.h"

#ifndef static_assert
#define static_assert(a, b) do { switch (0) case 0: case (a): ; } while (0)
#endif

#define NINODES 200

// Disk layout:
// [ boot block | sb block | log | inode blocks | free bit map | data blocks ]

int nbitmap = FSSIZE/(BSIZE*8) + 1;
int ninodeblocks = NINODES / IPB + 1;
int nlog = LOGSIZE;
int nmeta;    // Number of meta blocks (boot, sb, nlog, inode, bitmap)
int nblocks;  // Number of data blocks

int fsfd;
struct superblock sb;
char zeroes[BSIZE];
uint freeinode = 1;
uint freeblock;


void balloc(int);
void wsect(uint, void*);
void winode(uint, struct dinode*);
void rinode(uint inum, struct dinode *ip);
void rsect(uint sec, void *buf);
uint ialloc(ushort type);
void iappend(uint inum, void *p, int n);
void die(const char *);

// convert to riscv byte order
ushort
xshort(ushort x)
{
  ushort y;
  uchar *a = (uchar*)&y;
  a[0] = x;
  a[1] = x >> 8;
  return y;
}

uint
xint(uint x)
{
  uint y;
  uchar *a = (uchar*)&y;
  a[0] = x;
  a[1] = x >> 8;
  a[2] = x >> 16;
  a[3] = x >> 24;
  return y;
}

int
main(int argc, char *argv[])
{
  int i, cc, fd;
  int in_inode, in_bin, in_etc, in_demos;
  uint rootino, binino, etcino, demosino;
  uint inum, off;
  struct dirent de;
  char buf[BSIZE];
  struct dinode din;


  static_assert(sizeof(int) == 4, "Integers must be 4 bytes!");

  if(argc < 2){
    fprintf(stderr, "Usage: mkfs <fs.img> <kernel image> <files...\n");
    exit(1);
  }

  assert((BSIZE % sizeof(struct dinode)) == 0);
  assert((BSIZE % sizeof(struct dirent)) == 0);

  fsfd = open(argv[1], O_RDWR|O_CREAT|O_TRUNC, 0666);
  if(fsfd < 0)
    die(argv[1]);

  // 1 fs block = 1 disk sector
  nmeta = 2 + nlog + ninodeblocks + nbitmap;
  nblocks = FSSIZE - nmeta;

  sb.magic = FSMAGIC;
  sb.size = xint(FSSIZE);
  sb.nblocks = xint(nblocks);
  sb.ninodes = xint(NINODES);
  sb.nlog = xint(nlog);
  sb.logstart = xint(2);
  sb.inodestart = xint(2+nlog);
  sb.bmapstart = xint(2+nlog+ninodeblocks);

  printf("nmeta %d (boot, super, log blocks %u inode blocks %u, bitmap blocks %u) blocks %d total %d\n",
         nmeta, nlog, ninodeblocks, nbitmap, nblocks, FSSIZE);

  freeblock = nmeta;     // the first free block that we can allocate

  for(i = 0; i < FSSIZE; i++)
    wsect(i, zeroes);

  memset(buf, 0, sizeof(buf));
  memmove(buf, &sb, sizeof(sb));
  wsect(1, buf);

  rootino = ialloc(T_DIR);
  assert(rootino == ROOTINO);

  bzero(&de, sizeof(de));
  de.inum = xshort(rootino);
  strcpy(de.name, ".");
  iappend(rootino, &de, sizeof(de));
  bzero(&de, sizeof(de));
  de.inum = xshort(rootino);
  strcpy(de.name, "..");
  iappend(rootino, &de, sizeof(de));

  // copy kernel file
  if((fd = open(argv[2], 0)) < 0)
    die(argv[2]);
  inum = ialloc(T_FILE);
  bzero(&de, sizeof(de));
  de.inum = xshort(inum);
  strncpy(de.name, "xv6", DIRSIZ);
  iappend(rootino, &de, sizeof(de));
  while((cc = read(fd, buf, sizeof(buf))) > 0)
    iappend(inum, buf, cc);
  close(fd);

  // create /bin
  binino = ialloc(T_DIR);
  bzero(&de, sizeof(de));
  de.inum = xshort(binino);
  strcpy(de.name, "bin");
  iappend(rootino, &de, sizeof(de));
  bzero(&de, sizeof(de));
  de.inum = xshort(binino);
  strcpy(de.name, ".");
  iappend(binino, &de, sizeof(de));
  bzero(&de, sizeof(de));
  de.inum = xshort(rootino);
  strcpy(de.name, "..");
  iappend(binino, &de, sizeof(de));

  // create /etc
  etcino = ialloc(T_DIR);
  bzero(&de, sizeof(de));
  de.inum = xshort(etcino);
  strcpy(de.name, "etc");
  iappend(rootino, &de, sizeof(de));
  bzero(&de, sizeof(de));
  de.inum = xshort(etcino);
  strcpy(de.name, ".");
  iappend(etcino, &de, sizeof(de));
  bzero(&de, sizeof(de));
  de.inum = xshort(rootino);
  strcpy(de.name, "..");
  iappend(etcino, &de, sizeof(de));

  // create /demos
  demosino = ialloc(T_DIR);
  bzero(&de, sizeof(de));
  de.inum = xshort(demosino);
  strcpy(de.name, "demos");
  iappend(rootino, &de, sizeof(de));
  bzero(&de, sizeof(de));
  de.inum = xshort(demosino);
  strcpy(de.name, ".");
  iappend(demosino, &de, sizeof(de));
  bzero(&de, sizeof(de));
  de.inum = xshort(rootino);
  strcpy(de.name, "..");
  iappend(demosino, &de, sizeof(de));

  for(i = 3; i < argc; i++){
    char *shortname;
    if(strncmp(argv[i], "user/bin/", 9) == 0) {
      shortname = argv[i] + 9;
      in_bin = 1;
      in_etc = 0;
      in_demos = 0;
    } else if(strncmp(argv[i], "user/etc/", 9) == 0) {
      shortname = argv[i] + 9;
      in_bin = 0;
      in_etc = 1;
      in_demos = 0;
    } else if(strncmp(argv[i], "user/demos/", 11) == 0) {
      shortname = argv[i] + 11;
      in_bin = 0;
      in_etc = 0;
      in_demos = 1;
    } else if(strncmp(argv[i], "user/", 5) == 0) {
      shortname = argv[i] + 5;
      in_bin = 0;
      in_etc = 0;
      in_demos = 0;
    } else if(strncmp(argv[i], "kernel/", 7) == 0) {
      shortname = argv[i] + 7;
      in_bin = 0;
      in_etc = 0;
      in_demos = 0;
    } else {
      shortname = argv[i];
      in_bin = 0;
      in_etc = 0;
      in_demos = 0;
    }

    if (in_bin)
      in_inode = binino;
    else if (in_etc)
      in_inode = etcino;
    else if (in_demos)
      in_inode = demosino;
    else
      in_inode = rootino;

    assert(index(shortname, '/') == 0);

    if((fd = open(argv[i], 0)) < 0)
      die(argv[i]);

    // Skip leading _ in name when writing to file system.
    // The binaries are named _rm, _cat, etc. to keep the
    // build operating system from trying to execute them
    // in place of system binaries like rm and cat.
    if(shortname[0] == '_')
      shortname += 1;

    inum = ialloc(T_FILE);

    bzero(&de, sizeof(de));
    de.inum = xshort(inum);
    strncpy(de.name, shortname, DIRSIZ);
    iappend(in_inode, &de, sizeof(de));

    while((cc = read(fd, buf, sizeof(buf))) > 0)
      iappend(inum, buf, cc);

    close(fd);
  }

  // fix size of root inode dir
  rinode(rootino, &din);
  off = xint(din.size);
  off = ((off/BSIZE) + 1) * BSIZE;
  din.size = xint(off);
  winode(rootino, &din);

  balloc(freeblock);

  exit(0);
}

void
wsect(uint sec, void *buf)
{
  if(lseek(fsfd, sec * BSIZE, 0) != sec * BSIZE)
    die("lseek");
  if(write(fsfd, buf, BSIZE) != BSIZE)
    die("write");
}

void
winode(uint inum, struct dinode *ip)
{
  char buf[BSIZE];
  uint bn;
  struct dinode *dip;

  bn = IBLOCK(inum, sb);
  rsect(bn, buf);
  dip = ((struct dinode*)buf) + (inum % IPB);
  *dip = *ip;
  wsect(bn, buf);
}

void
rinode(uint inum, struct dinode *ip)
{
  char buf[BSIZE];
  uint bn;
  struct dinode *dip;

  bn = IBLOCK(inum, sb);
  rsect(bn, buf);
  dip = ((struct dinode*)buf) + (inum % IPB);
  *ip = *dip;
}

void
rsect(uint sec, void *buf)
{
  if(lseek(fsfd, sec * BSIZE, 0) != sec * BSIZE)
    die("lseek");
  if(read(fsfd, buf, BSIZE) != BSIZE)
    die("read");
}

uint
ialloc(ushort type)
{
  uint inum = freeinode++;
  struct dinode din;

  bzero(&din, sizeof(din));
  din.type = xshort(type);
  din.nlink = xshort(1);
  din.size = xint(0);
  winode(inum, &din);
  return inum;
}

void
balloc(int used)
{
  uchar buf[BSIZE];
  int i;

  printf("balloc: first %d blocks have been allocated\n", used);
  assert(used < BSIZE*8);
  bzero(buf, BSIZE);
  for(i = 0; i < used; i++){
    buf[i/8] = buf[i/8] | (0x1 << (i%8));
  }
  printf("balloc: write bitmap block at sector %d\n", sb.bmapstart);
  wsect(sb.bmapstart, buf);
}

#define min(a, b) ((a) < (b) ? (a) : (b))

void
iappend(uint inum, void *xp, int n)
{
  char *p = (char*)xp;
  uint fbn, off, n1;
  struct dinode din;
  char buf[BSIZE];
  uint indirect[NINDIRECT];
  uint x, y; //y is used to save the last indirect block's block number

  rinode(inum, &din);
  off = xint(din.size);
  // printf("append inum %d at off %d sz %d\n", inum, off, n);
  while(n > 0){

    fbn = off / BSIZE;

    //assert(fbn < MAXFILE);

    int NI_count = 1;
    int NI_size = NINDIRECT - 1; //128 - 1 = 127

    // find which indirect block bn is located in
    while(fbn >= NI_size){
      fbn = fbn - NI_size;
      NI_count++;
    }

    int new_block = 0;

    // find the block number of the INDIRECT BLOCK
    // if need to allocate a new indirect block, then need to save the block's block number in y
    int i;
    for(i = 1; i <= NI_count; i++){
      if(i == 1){

        // read the sector that contains the indirect table into indirect
        if(NI_count == 1) {
          if(xint(din.addrs[0]) == 0){
            din.addrs[0] = xint(freeblock++);
            new_block = 1;
          }
        }

        rsect(xint(din.addrs[0]),(char*)indirect);
      }
      else{
        if(indirect[NI_size] == 0){
          indirect[NI_size] = xint(freeblock++);
          new_block = 1;

          if(i == 2) wsect(xint(din.addrs[0]),(char*)indirect);
          else wsect(y,(char*)indirect);
        }

        y = xint(indirect[NI_size]);

        // read the sector that contains the indirect table into indirect
        rsect(y,(char*)indirect);
      }

      if(new_block) {
        indirect[NI_size] = 0;
        new_block = 0;
      }
    }

    if(indirect[fbn] == 0){
      indirect[fbn] = xint(freeblock++);
      if(NI_count == 1) wsect(xint(din.addrs[0]), (char*)indirect);
      else wsect(y, (char*)indirect);
    }

    //get the sector number
    x = xint(indirect[fbn]);

    //restore fbn
    fbn = off/BSIZE;

    n1 = min(n, (fbn + 1) * BSIZE - off);
    rsect(x, buf);
    bcopy(p, buf + off - (fbn * BSIZE), n1);
    wsect(x, buf);
    n -= n1;
    off += n1;
    p += n1;
  }
  din.size = xint(off);
  winode(inum, &din);
}

void
die(const char *s)
{
  perror(s);
  exit(1);
}
