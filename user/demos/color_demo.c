#include "kernel/types.h"
#include "user/user.h"

int main(int argc, char *argv[]) {
  printf("\e[31mc\e[32mo\e[33ml\e[34mo\e[35mr\e[0m demo\n");
  printf("the xv6 framebuffer console supports standard ANSI escape codes\n");
  printf("in order to control the foreground and background colors\n\n");

  printf("\e[34mhello xv6 world!\n");
  printf("\e[35mhello xv6 world!\n");
  printf("\e[37mhello xv6 world!\n");
  printf("\e[35mhello xv6 world!\n");
  printf("\e[34mhello xv6 world!\n");

  printf("\e[0m\n");
  exit(0);
}
