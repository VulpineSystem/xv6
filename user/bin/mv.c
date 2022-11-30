#include "kernel/types.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  if(argc != 3){
    fprintf(2, "Usage: mv old new\n");
    exit(1);
  }
  if((link(argv[1], argv[2]) < 0) || (unlink(argv[1]) < 0))
    fprintf(2, "mv: link/unlink failed\n");

  exit(0);
}
