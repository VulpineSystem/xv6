#include "types.h"
#include "riscv.h"
#include "memlayout.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "fs.h"
#include "file.h"
#include "memlayout.h"
#include "defs.h"
#include "proc.h"

#define LSHIFT_PRESS   0x2A
#define LSHIFT_RELEASE 0xAA
#define RSHIFT_PRESS   0x36
#define RSHIFT_RELEASE 0xB6
#define LCTRL_PRESS    0x1D
#define LCTRL_RELEASE  0x9D

struct {
  struct spinlock lock;
} kbd;

int shift = 0;
int ctrl = 0;

char scancode_map[128] = {
  0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
  '\t', // Tab
  'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
  0, // Control
  'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,
  '*',
  0,  // Alt
  ' ', // Space
  0, // Caps lock
  0, // F1
  0, 0, 0, 0, 0, 0, 0, 0,
  0, // F10
  0, // Num lock
  0, // Scroll lock
  0, // Home
  0, // Up arrow
  0, // Page Up
  '-',
  0, // Left arrow
  0,
  0, // Right arrow
  '+',
  0, // End
  0, // Down arrow
  0, // Page Down
  0, // Insert
  0, // Delete
  0, 0, 0,
  0, // F11
  0, // F12
  0, // All other keys are undefined
};

char scancode_map_shift[128] = {
  0,  27, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',
  '\t', // Tab
  'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',
  0, // Control
  'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~', 0, '|',
  'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0,
  '*',
  0,  // Alt
  ' ', // Space
  0, // Caps lock
  0, // F1
  0, 0, 0, 0, 0, 0, 0, 0,
  0, // F10
  0, // Num lock
  0, // Scroll lock
  0, // Home
  0, // Up arrow
  0, // Page Up
  '-',
  0, // Left arrow
  0,
  0, // Right arrow
  '+',
  0, // End
  0, // Down arrow
  0, // Page Down
  0, // Insert
  0, // Delete
  0, 0, 0,
  0, // F11
  0, // F12
  0, // All other keys are undefined
};

//
// user read()s from the keyboard go here.
//
int
keyboardread(int user_dst, uint64 dst, uint off, int n)
{
  int scancode = read_keyboard_raw();
  int r = 0;
  for (; n > 0; n--) {
    r = either_copyout(user_dst, dst++, &scancode, 1);
    if (r == -1)
      break;
  }
  return r;
}

void
keyboardinit(void)
{
  initlock(&kbd.lock, "kbd");

  // connect read and write system calls
  devsw[KEYBOARD].read = keyboardread;
  devsw[KEYBOARD].write = 0;
}

int
read_keyboard_raw(void)
{
  int scancode = *(int *) KBD0;
  if (scancode == LSHIFT_PRESS || scancode == RSHIFT_PRESS)
    shift = 1;
  else if (scancode == LSHIFT_RELEASE || scancode == RSHIFT_RELEASE)
    shift = 0;
  else if (scancode == LCTRL_PRESS)
    ctrl = 1;
  else if (scancode == LCTRL_RELEASE)
    ctrl = 0;

  return scancode;
}

int
read_keyboard(void)
{
  int scancode = *(int *) KBD0;
  if (scancode == LSHIFT_PRESS || scancode == RSHIFT_PRESS) {
    shift = 1;
    scancode = 0;
  } else if (scancode == LSHIFT_RELEASE || scancode == RSHIFT_RELEASE) {
    shift = 0;
    scancode = 0;
  } else if (scancode == LCTRL_PRESS) {
    ctrl = 1;
    scancode = 0;
  } else if (scancode == LCTRL_RELEASE) {
    ctrl = 0;
    scancode = 0;
  }

  // only care about make scancodes, not break scancodes
  if (scancode & 0x80)
    scancode = 0;

  return scancode;
}

int
scancode_to_ascii(int scancode)
{
  if (shift)
    return scancode_map_shift[scancode];
  else if (ctrl)
    return scancode ? scancode_map_shift[scancode] - '@' : 0;
  else
    return scancode_map[scancode];
}
