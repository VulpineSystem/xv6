#include "kernel/types.h"
#include "kernel/fcntl.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  char buf[512];
  int src_fd, dest_fd, r, w = -1;
  char *src;
  char *dest;

  if(argc != 3){
    fprintf(2, "Usage: cp source destination\n");
    exit(0);
  }

  src = argv[1];
  dest = argv[2];

  // open source and destination files
  if((src_fd = open(src, O_RDONLY)) < 0){
    fprintf(2, "cp: cannot open source %s\n", src);
    exit(1);
  }
  if((dest_fd = open(dest, O_CREATE|O_WRONLY)) < 0){
    fprintf(2, "cp: cannot open destination %s\n", dest);
    exit(1);
  }

  // loop over the contents of the source and write it to the destination
  while((r = read(src_fd, buf, sizeof(buf))) > 0){
    w = write(dest_fd, buf, r);
    if (w != r || w < 0)
      break;
  }
  if (r < 0 || w < 0)
    fprintf(2, "cp: error copying %s to %s\n", src, dest);

  close(src_fd);
  close(dest_fd);

  exit(0);
}
