#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "font.h"

#define CHAR_WIDTH 8
#define CHAR_HEIGHT 16
#define CONSOLE_WIDTH 80
#define CONSOLE_HEIGHT 30
#define FRAMEBUFFER_WIDTH 640
#define FRAMEBUFFER_HEIGHT 480

static uint32 (*framebuffer)[FRAMEBUFFER_HEIGHT][FRAMEBUFFER_WIDTH] = (void *) 0x80600000;

static char on_screen_buffer[CONSOLE_HEIGHT][CONSOLE_WIDTH];
static char update_buffer[CONSOLE_HEIGHT][CONSOLE_WIDTH];
uint console_x = 0;
uint console_y = 0;

static inline void
fb_console_draw_character(uint x, uint y, char character)
{
  uint32 *ptr = (void *) &unifont[CHAR_WIDTH * CHAR_HEIGHT * 4 * (uint) character];
  for (uint iy = y; iy < y + CHAR_HEIGHT; iy++) {
    for (uint ix = x; ix < x + CHAR_WIDTH; ix++) {
      (*framebuffer)[iy][ix] = *ptr++;
    }
  }
}

void
fb_console_redraw()
{
  for (uint y = 0; y < CONSOLE_HEIGHT; y++) {
    for (uint x = 0; x < CONSOLE_WIDTH; x++) {
      if (on_screen_buffer[y][x] != update_buffer[y][x]) {
        on_screen_buffer[y][x] = update_buffer[y][x];
        fb_console_draw_character(x * CHAR_WIDTH, y * CHAR_HEIGHT, update_buffer[y][x]);
        update_buffer[y][x] = 0;
      }
    }
  }
}

void
fb_console_redraw_line()
{
  for (uint x = 0; x < CONSOLE_WIDTH; x++) {
    if (on_screen_buffer[console_y][x] != update_buffer[console_y][x]) {
      on_screen_buffer[console_y][x] = update_buffer[console_y][x];
      fb_console_draw_character(x * CHAR_WIDTH, console_y * CHAR_HEIGHT, update_buffer[console_y][x]);
    }
  }
}

void
fb_console_scroll()
{
  for (uint i = 1; i < CONSOLE_HEIGHT; i++) {
    memmove(&update_buffer[i - 1], &on_screen_buffer[i], CONSOLE_WIDTH);
  }
  memset(&update_buffer[CONSOLE_HEIGHT - 1], 0, CONSOLE_WIDTH);
  console_y = CONSOLE_HEIGHT - 1;
  fb_console_redraw();
}

void
fb_console_print_character(char character)
{
  // check for various characters
  if (character == 0) {
    // null
    return;
  } else if (character == '\b') {
    // backspace
    if (console_x > 0)
      console_x--;
    return;
  } else if (character == '\n') {
    // line feed
    fb_console_redraw_line();
    console_x = 0;
    console_y++;
    if (console_y >= CONSOLE_HEIGHT)
      fb_console_scroll();
    return;
  }

  // check if we are at the end of this line
  if (console_x >= CONSOLE_WIDTH) {
    // if so, redraw and increment to the next line
    fb_console_redraw_line();
    console_x = 0;
    console_y++;
  }

  // check if we need to scroll the console
  if (console_y >= CONSOLE_HEIGHT)
    fb_console_scroll();

  // set character in the update buffer
  update_buffer[console_y][console_x] = character;

  // increment X counter
  console_x++;
}
