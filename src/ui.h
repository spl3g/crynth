#ifndef UI_H_
#define UI_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#include <SDL3/SDL_scancode.h>
#include <SDL3/SDL_keycode.h>

#include "clay/clay.h"
#include "messages.h"

static const Clay_Color COLOR_BG = (Clay_Color){45, 53, 59, 255};
static const Clay_Color COLOR_BG_INTER = (Clay_Color){52, 63, 68, 255};
static const Clay_Color COLOR_FG = (Clay_Color){211, 198, 170, 255};
static const Clay_Color COLOR_FG_INTER = (Clay_Color){227, 212, 181, 255};

typedef struct {
  char letter;
  SDL_Keycode keycode;
  bool mouse_pressed;
  bool keyboard_pressed;
} KeyState;

typedef struct {
  message_queue *msg_queue;
  KeyState *keys;
  size_t keys_amount;
} UIData;

void draw_ui(UIData *ui_data);

#endif // UI_H_
