#ifndef UI_H_
#define UI_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#include <SDL3/SDL_scancode.h>
#include <SDL3/SDL_keycode.h>

#include "arena.h"

#include "clay_renderer_SDL3.h"
#include "clay.h"
#include "messages.h"

#include "defines.h"

static const Clay_Color COLOR_BG = (Clay_Color){45, 53, 59, 255};
static const Clay_Color COLOR_BG_INTER = (Clay_Color){52, 63, 68, 255};
static const Clay_Color COLOR_FG = (Clay_Color){211, 198, 170, 255};
static const Clay_Color COLOR_FG_INTER = (Clay_Color){227, 212, 181, 255};
static const Clay_Color COLOR_ACCENT = (Clay_Color){133, 146, 137, 255};

typedef struct {
  char letter;
  SDL_Keycode keycode;
  bool mouse_pressed;
  bool keyboard_pressed;
} KeyState;

typedef struct {
  float value;
  ParamType param_type;
  float range_start;
  float range_end;
} KnobInfo;

typedef struct {
  KnobInfo volume;
  KnobInfo attack;
  KnobInfo decay;
  KnobInfo sustain;
  KnobInfo release;
} KnobSettings;

typedef struct {
  Arena *arena;
  MessageQueue *msg_queue;
  float *wave_buffer;
  size_t wave_buffer_size;

  KeyState *keys;
  size_t keys_amount;
  KnobSettings *knob_settings;
  float scale;
} UIData;

typedef struct {
  UIData *ui_data;
  float start_angle;
  KnobInfo *info;
} UIKnobData;

void draw_ui(UIData *ui_data);

#endif // UI_H_
