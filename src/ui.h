#ifndef UI_H_
#define UI_H_

#include <stdbool.h>
#include <stddef.h>
#include "clay/clay.h"

static const Clay_Color COLOR_BG = (Clay_Color){45, 53, 59, 255};
static const Clay_Color COLOR_FG = (Clay_Color){211, 198, 170, 255};

void draw_ui(bool *pressed_keys, size_t keys_amount);

#endif // UI_H_
