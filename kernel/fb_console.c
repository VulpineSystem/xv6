#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "font.h"
#include "framebuffer.h"

#define FOREGROUND 0
#define BACKGROUND 1
#define CHAR_WIDTH 8
#define CHAR_HEIGHT 16
#define CONSOLE_WIDTH 80
#define CONSOLE_HEIGHT 30
#define DEFAULT_FOREGROUND_COLOR 8
#define DEFAULT_BACKGROUND_COLOR 0
#define MAX_ESC_CODE_PARAMETERS 8

static uint32 (*framebuffer)[FRAMEBUFFER_HEIGHT][FRAMEBUFFER_WIDTH] = (void *) 0x80600000;

static char on_screen_buffer[CONSOLE_HEIGHT][CONSOLE_WIDTH];
static char on_screen_color_buffer[CONSOLE_HEIGHT][CONSOLE_WIDTH][2];
static char update_buffer[CONSOLE_HEIGHT][CONSOLE_WIDTH];
static char update_color_buffer[CONSOLE_HEIGHT][CONSOLE_WIDTH][2];
static uint console_x = 0;
static uint console_y = 0;
static uint current_foreground_color_offset = DEFAULT_FOREGROUND_COLOR;
static uint current_background_color_offset = DEFAULT_BACKGROUND_COLOR;

static uint escape_code_parameters[MAX_ESC_CODE_PARAMETERS];
static uint escape_code_parameter_count = 0;
static int is_in_escape_code = 0;

static uint colors[9] = {
  0x2e1e1e, // black
  0xa88bf3, // red
  0xa1e3a6, // green
  0xafe2f9, // yellow
  0xfab489, // blue
  0xaca0eb, // magenta
  0xd5e294, // cyan
  0xf4d6cd, // white
  0xf4d6cd  // default
};

static inline void
fb_console_draw_character(uint x, uint y, char character, uint32 foreground_color, uint32 background_color)
{
  uint32 color;
  uint32 *ptr = (void *) &unifont[CHAR_WIDTH * CHAR_HEIGHT * 4 * (uint) character];
  for (uint iy = y; iy < y + CHAR_HEIGHT; iy++) {
    for (uint ix = x; ix < x + CHAR_WIDTH; ix++) {
      if (*ptr++ == 0xFFFFFFFF)
        color = foreground_color;
      else
        color = background_color;
      color |= 0xFF000000;
      (*framebuffer)[iy][ix] = color;
    }
  }
}

void
fb_console_redraw(void)
{
  uint foreground_color_offset;
  uint background_color_offset;
  for (uint y = 0; y < CONSOLE_HEIGHT; y++) {
    for (uint x = 0; x < CONSOLE_WIDTH; x++) {
      if ((on_screen_buffer[y][x] != update_buffer[y][x]) ||
          (on_screen_color_buffer[y][x][FOREGROUND] != update_color_buffer[y][x][FOREGROUND]) ||
          (on_screen_color_buffer[y][x][BACKGROUND] != update_color_buffer[y][x][BACKGROUND])) {
        on_screen_buffer[y][x] = update_buffer[y][x];
        on_screen_color_buffer[y][x][FOREGROUND] = update_color_buffer[y][x][FOREGROUND];
        on_screen_color_buffer[y][x][BACKGROUND] = update_color_buffer[y][x][BACKGROUND];
        foreground_color_offset = update_color_buffer[y][x][FOREGROUND];
        background_color_offset = update_color_buffer[y][x][BACKGROUND];
        fb_console_draw_character(x * CHAR_WIDTH, y * CHAR_HEIGHT, update_buffer[y][x], colors[foreground_color_offset], colors[background_color_offset]);
        update_buffer[y][x] = 0;
        update_color_buffer[y][x][FOREGROUND] = 0;
        update_color_buffer[y][x][BACKGROUND] = 0;
      }
    }
  }
}

void
fb_console_redraw_line(void)
{
  uint foreground_color_offset;
  uint background_color_offset;
  for (uint x = 0; x < CONSOLE_WIDTH; x++) {
    if ((on_screen_buffer[console_y][x] != update_buffer[console_y][x]) ||
        (on_screen_color_buffer[console_y][x][FOREGROUND] != update_color_buffer[console_y][x][FOREGROUND]) ||
        (on_screen_color_buffer[console_y][x][BACKGROUND] != update_color_buffer[console_y][x][BACKGROUND])) {
      on_screen_buffer[console_y][x] = update_buffer[console_y][x];
      on_screen_color_buffer[console_y][x][FOREGROUND] = update_color_buffer[console_y][x][FOREGROUND];
      on_screen_color_buffer[console_y][x][BACKGROUND] = update_color_buffer[console_y][x][BACKGROUND];
      foreground_color_offset = update_color_buffer[console_y][x][FOREGROUND];
      background_color_offset = update_color_buffer[console_y][x][BACKGROUND];
      fb_console_draw_character(x * CHAR_WIDTH, console_y * CHAR_HEIGHT, update_buffer[console_y][x], colors[foreground_color_offset], colors[background_color_offset]);
    }
  }
}

void
fb_console_scroll(void)
{
  for (uint i = 1; i < CONSOLE_HEIGHT; i++) {
    memmove(&update_buffer[i - 1], &on_screen_buffer[i], CONSOLE_WIDTH);
    memmove(&update_color_buffer[i - 1], &on_screen_color_buffer[i], CONSOLE_WIDTH * 2);
  }
  memset(&update_buffer[CONSOLE_HEIGHT - 1], 0, CONSOLE_WIDTH);
  memset(&update_color_buffer[CONSOLE_HEIGHT - 1], 0, CONSOLE_WIDTH * 2);
  console_y = CONSOLE_HEIGHT - 1;
  fb_console_redraw();
}

void
fb_console_handle_esc_code(char character)
{
  if (character >= '0' && character <= '9') {
    escape_code_parameters[escape_code_parameter_count] *= 10;
    escape_code_parameters[escape_code_parameter_count] += character - '0';
    return;
  }

  if (character == '[') {
    return;
  } else if (character == ';') {
    escape_code_parameter_count++;
    if (escape_code_parameter_count >= MAX_ESC_CODE_PARAMETERS)
      escape_code_parameter_count = 0;
    return;
  }

  is_in_escape_code = 0;

  // TODO: implement the rest of the control codes
  if (character == 'm') {
    // set color
    for (int i = 0; i <= escape_code_parameter_count; i++) {
      uint parameter = escape_code_parameters[i];
      if (parameter == 0) {
        // reset colors
        current_foreground_color_offset = DEFAULT_FOREGROUND_COLOR;
        current_background_color_offset = DEFAULT_BACKGROUND_COLOR;
      } else if (parameter == 39) {
        // reset foreground color
        current_foreground_color_offset = DEFAULT_FOREGROUND_COLOR;
      } else if (parameter == 49) {
        // reset background color
        current_background_color_offset = DEFAULT_BACKGROUND_COLOR;
      } else if (parameter >= 30 && parameter <= 37) {
        // set foreground color
        current_foreground_color_offset = parameter - 30;
      } else if (parameter >= 40 && parameter <= 47) {
        // set background color
        current_background_color_offset = parameter - 40;
      }
    }
  }
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
  } else if (character == '\e') {
    is_in_escape_code = 1;
    escape_code_parameter_count = 0;
    for (int i = 0; i < MAX_ESC_CODE_PARAMETERS; i++) {
      escape_code_parameters[i] = 0;
    }
    return;
  }

  if (is_in_escape_code) {
    fb_console_handle_esc_code(character);
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

  // set character and colors in the update buffers
  update_buffer[console_y][console_x] = character;
  update_color_buffer[console_y][console_x][FOREGROUND] = current_foreground_color_offset;
  update_color_buffer[console_y][console_x][BACKGROUND] = current_background_color_offset;

  // increment X counter
  console_x++;
}
