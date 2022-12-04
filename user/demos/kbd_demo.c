#include "kernel/types.h"
#include "kernel/fcntl.h"
#include "user/user.h"

#define ESCAPE 0x01

int done = 0;
int scancode;

int main(int argc, char *argv[]) {
  printf("keyboard demo: press keys to see their scancodes\n");
  printf("press esc to quit\n");

  int kbd = open("/dev/keyboard", O_RDONLY);
  if (!kbd) {
    printf("failed to open /dev/keyboard\n");
    exit(1);
  }

  while (!done) {
    if (read(kbd, &scancode, 1) < 0) {
      printf("error while reading /dev/keyboard\n");
      break;
    }

    if (scancode == ESCAPE)
      break;

    if (scancode)
      printf("scancode: %d\n", scancode);
  }

  close(kbd);
  exit(0);
}
