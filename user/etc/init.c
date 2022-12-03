// init: The initial user-level program

#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/spinlock.h"
#include "kernel/sleeplock.h"
#include "kernel/fs.h"
#include "kernel/file.h"
#include "user/user.h"
#include "kernel/fcntl.h"

char *argv[] = { "/bin/sh", 0 };

int
main(void)
{
  int pid, wpid;

  mkdir("/dev");

  if(open("/dev/console", O_RDWR) < 0){
    mknod("/dev/console", CONSOLE, 0);
    open("/dev/console", O_RDWR);
  }
  dup(0);  // stdout
  dup(0);  // stderr

  if(open("/dev/framebuffer", O_RDWR) < 0){
    mknod("/dev/framebuffer", FRAMEBUFFER, 0);
    open("/dev/framebuffer", O_RDWR);
  }

  if(open("/dev/keyboard", O_RDWR) < 0){
    mknod("/dev/keyboard", KEYBOARD, 0);
    open("/dev/keyboard", O_RDWR);
  }

  for(;;){
    printf("init: starting /bin/sh\n");
    pid = fork();
    if(pid < 0){
      printf("init: fork failed\n");
      exit(1);
    }
    if(pid == 0){
      exec("/bin/sh", argv);
      printf("init: exec /bin/sh failed\n");
      exit(1);
    }

    for(;;){
      // this call to wait() returns if the shell exits,
      // or if a parentless process exits.
      wpid = wait((int *) 0);
      if(wpid == pid){
        // the shell exited; restart it.
        break;
      } else if(wpid < 0){
        printf("init: wait returned an error\n");
        exit(1);
      } else {
        // it was a parentless process; do nothing.
      }
    }
  }
}
