#include "types.h"
#include "riscv.h"
#include "memlayout.h"

#define LSHIFT_PRESS   0x2A
#define LSHIFT_RELEASE 0xAA
#define RSHIFT_PRESS   0x36
#define RSHIFT_RELEASE 0xB6

int shift = 0;

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

int
read_keyboard()
{
  int scancode = *(int *) KBD0;
  if (scancode == LSHIFT_PRESS || scancode == RSHIFT_PRESS) {
    shift = 1;
    scancode = 0;
  } else if (scancode == LSHIFT_RELEASE || scancode == RSHIFT_RELEASE) {
    shift = 0;
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
  else
    return scancode_map[scancode];
}
