#include "ui.h"

void handle_key_press(Clay_ElementId element_id, Clay_PointerData pointer_info, intptr_t user_data) {
  UIData *ui_data = (UIData *)user_data;
  int idx = element_id.offset;

  bool pressed = ui_data->keys[idx].mouse_pressed;

  if (!pressed && (pointer_info.state == CLAY_POINTER_DATA_PRESSED_THIS_FRAME
									  || pointer_info.state == CLAY_POINTER_DATA_PRESSED)) {
	mqueue_push(ui_data->msg_queue, (synth_message){
		.type = MSG_NOTE_ON,
		.note = {
		  .note_id = idx,
		},
	});
	ui_data->keys[idx].mouse_pressed = true;
  }

  if (pressed && (pointer_info.state == CLAY_POINTER_DATA_RELEASED_THIS_FRAME
								 || pointer_info.state == CLAY_POINTER_DATA_RELEASED)) {
	if (!ui_data->keys[idx].keyboard_pressed) {
	  mqueue_push(ui_data->msg_queue, (synth_message){
		  .type = MSG_NOTE_OFF,
		  .note = {
			.note_id = idx,
		  },
		});
	}
	ui_data->keys[idx].mouse_pressed = false;
  }
}

void draw_white_key(size_t idx, UIData *ui_data) {
  CLAY(CLAY_IDI("key_container", idx)) {
	bool mouse_pressed = ui_data->keys[idx].mouse_pressed;
	bool keyboard_pressed = ui_data->keys[idx].keyboard_pressed;
	bool hovered = Clay_Hovered();

	if (!hovered && mouse_pressed && !keyboard_pressed) {
	  mqueue_push(ui_data->msg_queue, (synth_message){
	    .type = MSG_NOTE_OFF,
	    .note = {
		  .note_id = idx,
	    },
	  });

	  mouse_pressed = false;
	  ui_data->keys[idx].mouse_pressed = false;
	}

	Clay_OnHover(handle_key_press, (intptr_t)ui_data);


	Clay_Color fill_color;
	Clay_Color border_color;

	if (mouse_pressed || keyboard_pressed) {
	  fill_color = COLOR_BG;
	  border_color = COLOR_FG;
	} else if (hovered) {
	  fill_color = COLOR_FG_INTER;
	  border_color = COLOR_FG_INTER;
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
}

void draw_black_key(size_t idx, UIData *ui_data) {
  CLAY(CLAY_IDI("key_container", idx), {
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
	}) {
	bool mouse_pressed = ui_data->keys[idx].mouse_pressed;
	bool keyboard_pressed = ui_data->keys[idx].keyboard_pressed;
	bool hovered = Clay_Hovered();

	if (!hovered && mouse_pressed && !keyboard_pressed) {
	  mqueue_push(ui_data->msg_queue, (synth_message){
		  .type = MSG_NOTE_OFF,
		  .note = {
			.note_id = idx,
		  },
		});

	  mouse_pressed = false;
	  ui_data->keys[idx].mouse_pressed = false;
	}

	Clay_OnHover(handle_key_press, (intptr_t)ui_data);

	Clay_Color fill_color;
	Clay_Color border_color;

	if (keyboard_pressed || mouse_pressed) {
	  fill_color = COLOR_FG;
	  border_color = COLOR_BG;
	} else if (hovered) {
	  fill_color = COLOR_BG_INTER;
	  border_color = COLOR_FG;
	} else {
	  fill_color = COLOR_BG;
	  border_color = COLOR_FG;
	}
	CLAY(CLAY_IDI("black_key", idx), {
      .layout = {
	    .sizing = {CLAY_SIZING_FIXED(25), .height = CLAY_SIZING_FIXED(65)},
	  },
	  
	  .backgroundColor = fill_color,
      .border = { .width = {1, 1, 0, 1, 0}, .color = border_color},
	});
  }
}

void draw_keyboard(UIData *ui_data) {
  CLAY(CLAY_ID("keyboard"), {
	  .layout = {
		.layoutDirection = CLAY_LEFT_TO_RIGHT,
	  },
  }) {
	for (size_t i = 0; i < ui_data->keys_amount; i++) {
	  size_t key_idx = i % 12;
	  if (key_idx <= 4) {
		if (i % 2 == 0) {
		  draw_white_key(i, ui_data);
		} else {
		  draw_black_key(i, ui_data);
		}
	  } else {
		if (i % 2 == 0) {
		  draw_black_key(i, ui_data);
		} else {
		  draw_white_key(i, ui_data);
		}
	  }
	}
  }
}

void draw_screen() {}
void draw_nob() {}

void draw_ui(UIData *ui_data) {
  CLAY(CLAY_ID("outer_container"), {
	  .layout = {
		.sizing = {CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0)},
		.padding = CLAY_PADDING_ALL(16),
		.childGap = 16,
		.childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER},
	  },
	  .backgroundColor = COLOR_BG,
	}) {
	/* CLAY(CLAY_ID("app_container"), { */
	/* 	.layout = { */
	/* 	  .sizing = {CLAY_SIZING_PERCENT(0.75), CLAY_SIZING_PERCENT(0.75)} */
	/* 	}, */
	/* }) { */
	  draw_keyboard(ui_data);
	/* }; */
  };
}
