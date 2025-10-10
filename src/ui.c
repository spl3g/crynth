#include "ui.h"

void draw_white_key(size_t idx, bool pressed) {
  Clay_Color fill_color;
  Clay_Color border_color;

  if (pressed) {
	fill_color = COLOR_BG;
	border_color = COLOR_FG;
  } else {
	fill_color = COLOR_FG;
	border_color = COLOR_FG;
  }
  CLAY(CLAY_IDI("white_key", idx), {
	.layout = {
	  .sizing = {CLAY_SIZING_FIXED(40), .height = CLAY_SIZING_FIXED(100)},
	},
	.backgroundColor = fill_color,
	.border = { .width = {1, 1, 1, 1, 0}, .color = border_color},
  });  
}

void draw_black_key(size_t idx, bool pressed) {
  Clay_Color fill_color;
  Clay_Color border_color;

  if (pressed) {
	fill_color = COLOR_FG;
	border_color = COLOR_BG;
  } else {
	fill_color = COLOR_BG;
	border_color = COLOR_FG;
  }
  CLAY(CLAY_IDI("black_key", idx), {
	.layout = {
	  .sizing = {CLAY_SIZING_FIXED(25), .height = CLAY_SIZING_FIXED(65)},
	},

	.floating = {
	  .attachTo = CLAY_ATTACH_TO_ELEMENT_WITH_ID,
	  .parentId = CLAY_IDI("white_key", idx - 1).id,
	  .attachPoints = {
		.element = CLAY_ATTACH_POINT_CENTER_TOP,
		.parent = CLAY_ATTACH_POINT_RIGHT_TOP,
	  },
	  .offset = {
		.y = -1,
	  },
	},

	.backgroundColor = fill_color,
	.border = { .width = {1, 1, 0, 1, 0}, .color = border_color},
  });
}

void draw_keyboard(bool *pressed_keys, size_t keys_amount) {
  CLAY(CLAY_ID("keyboard"), {
	  .layout = {
		.layoutDirection = CLAY_LEFT_TO_RIGHT,
	  },
  }) {
	for (size_t i = 0; i < keys_amount; i++) {
	  bool pressed = pressed_keys[i];
	  size_t key_idx = i % 12;
	  if (key_idx <= 4) {
		if (i % 2 == 0) {
		  draw_white_key(i, pressed);
		} else {
		  draw_black_key(i, pressed);
		}
	  } else {
		if (i % 2 == 0) {
		  draw_black_key(i, pressed);
		} else {
		  draw_white_key(i, pressed);
		}
	  }
	}
  }
}

void draw_ui(bool *pressed_keys, size_t keys_amount) {
  CLAY(CLAY_ID("OuterContainer"), {
	  .layout = {
		.sizing = {CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0)},
		.padding = CLAY_PADDING_ALL(16),
		.childGap = 16,
		.childAlignment = {
		  .x = CLAY_ALIGN_X_CENTER,
		  .y = CLAY_ALIGN_Y_CENTER,
		},
	  },
	  .backgroundColor = COLOR_BG,
	}) {
	draw_keyboard(pressed_keys, keys_amount);
  };
}
