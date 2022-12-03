#include "kernel/types.h"
#include "user/user.h"

int main(int argc, char *argv[]) {
  printf("\e31mc\e32mo\e33ml\e34mo\e35mr\e0m demo\n");
  printf("the xv6 framebuffer console supports standard ANSI escape codes\n");
  printf("in order to control the foreground and background colors\n\n");

  printf("\e34mhello xv6 world!\n");
  printf("\e35mhello xv6 world!\n");
  printf("\e37mhello xv6 world!\n");
  printf("\e35mhello xv6 world!\n");
  printf("\e34mhello xv6 world!\n");

  printf("\e0m\n");
  exit(0);
}
