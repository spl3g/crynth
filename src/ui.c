#include "ui.h"

bool point_is_inside_circle(Clay_Vector2 point, Clay_BoundingBox circle) {
  float center_x = circle.x + circle.width / 2;
  float center_y = circle.y + circle.height / 2;
  float min_diameter = circle.width > circle.height ? circle.height : circle.width;
  float radius = min_diameter / 2;

  return (point.x - center_x) * (point.x - center_x) + (point.y - center_y) * (point.y - center_y) <= radius * radius;
}

float point_angle_on_circle(Clay_Vector2 point, Clay_BoundingBox circle) {
  float center_x = circle.x + circle.width / 2;
  float center_y = circle.y + circle.width / 2;

  float point_rad = atan2f(point.y - center_y, point.x - center_x);

  return point_rad * 180.0f / M_PI;
}

float point_value_on_circle(Clay_Vector2 point, Clay_BoundingBox circle, float start_angle) {
  float point_on_circle = point_angle_on_circle(point, circle);
  float value = fmodf((point_on_circle - start_angle + 360), 360) / 360;
  return value;
}


static inline float normalize_value(float value, float range_start, float range_end)
{
    if (range_end == range_start) return 0.0f; // avoid divide-by-zero
    float normalized = (value - range_start) / (range_end - range_start);
    if (normalized < 0.0f) normalized = 0.0f;
    if (normalized > 1.0f) normalized = 1.0f;
    return normalized;
}

static inline float denormalize_value(float normalized, float range_start, float range_end)
{
    if (normalized < 0.0f) normalized = 0.0f;
    if (normalized > 1.0f) normalized = 1.0f;
    return range_start + normalized * (range_end - range_start);
}


bool circle_hovered(void) {
  if (!Clay_Hovered()) {
	return false;
  }
  
  Clay_PointerData pointer = Clay_GetPointerState();

  Clay_ElementId element_id = Clay_GetCurrentElementId();
  Clay_ElementData element_data = Clay_GetElementData(element_id);

  return point_is_inside_circle(pointer.position, element_data.boundingBox);
}

void handle_knob_press(Clay_ElementId element_id, Clay_PointerData pointer_info, intptr_t user_data) {
  Clay_ElementData element_data = Clay_GetElementData(element_id);

  if (!point_is_inside_circle(pointer_info.position, element_data.boundingBox)) {
	return;
  }

  UIKnobData *knob = (UIKnobData *)user_data;
  UIData *ui_data = knob->ui_data;

  if (pointer_info.state == CLAY_POINTER_DATA_PRESSED_THIS_FRAME
	  || pointer_info.state == CLAY_POINTER_DATA_PRESSED) {
	
	float normalized_value = point_value_on_circle(pointer_info.position,
												   element_data.boundingBox,
												   knob->start_angle);
	float start = knob->info->range_start;
	float end = knob->info->range_end;
	float value = denormalize_value(normalized_value, start, end);

	knob->info->value = value;
	mqueue_push(ui_data->msg_queue, (SynthMessage){
	  .type = MSG_PARAM_CHANGE,
	  .param_change = {
		.param_type = knob->info->param_type,
		.value = value,
	  },
	});
  }
}

void handle_key_press(Clay_ElementId element_id, Clay_PointerData pointer_info, intptr_t user_data) {
  UIData *ui_data = (UIData *)user_data;
  int idx = element_id.offset;

  bool pressed = ui_data->keys[idx].mouse_pressed;

  if (!pressed && (pointer_info.state == CLAY_POINTER_DATA_PRESSED_THIS_FRAME
									  || pointer_info.state == CLAY_POINTER_DATA_PRESSED)) {
	mqueue_push(ui_data->msg_queue, (SynthMessage){
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
	  mqueue_push(ui_data->msg_queue, (SynthMessage){
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
	  mqueue_push(ui_data->msg_queue, (SynthMessage){
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
	    .sizing = {CLAY_SIZING_FIXED(40 * ui_data->scale), CLAY_SIZING_FIXED(100 * ui_data->scale)},
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
		/* .offset = { */
		/*   .y = -1, */
		/* }, */
	  },
	}) {
	bool mouse_pressed = ui_data->keys[idx].mouse_pressed;
	bool keyboard_pressed = ui_data->keys[idx].keyboard_pressed;
	bool hovered = Clay_Hovered();

	if (!hovered && mouse_pressed && !keyboard_pressed) {
	  mqueue_push(ui_data->msg_queue, (SynthMessage){
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
	    .sizing = {CLAY_SIZING_FIXED(25 * ui_data->scale), CLAY_SIZING_FIXED(65 * ui_data->scale)},
	  },
	  .backgroundColor = fill_color,
      .border = { .width = {1, 1, 0, 1, 0}, .color = border_color},
	});
  }
}

void draw_keyboard(UIData *ui_data) {
  CLAY(CLAY_ID("keyboard_container"), {
	  .layout = {
		.sizing = {CLAY_SIZING_FIXED(280 * ui_data->scale), CLAY_SIZING_FIXED(100 * ui_data->scale)},
		.layoutDirection = CLAY_LEFT_TO_RIGHT,
		.childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER},
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

void draw_knob(Clay_ElementId id, UIData *ui_data, KnobInfo* knob_info, Clay_SizingAxis outer_size, Clay_SizingAxis inner_size) {
  CLAY(id) {
	bool hovered = circle_hovered();
	UIKnobData *knob_data = arena_alloc(ui_data->arena, sizeof(UIKnobData));
	knob_data->ui_data = ui_data;
	knob_data->start_angle = -90;
	knob_data->info = knob_info;
	float value = normalize_value(knob_info->value, knob_info->range_start, knob_info->range_end);
	
	
	Clay_OnHover(handle_knob_press, (intptr_t)knob_data);

	CustomElementData *knob_element_data = arena_alloc(ui_data->arena, sizeof(CustomElementData));
	knob_element_data->type = CUSTOM_ELEMENT_TYPE_CIRCLE;
	knob_element_data->circle = (CircleData){
	  .start_angle = -90,
	  .value = value,
	};
	CLAY_AUTO_ID({
		.layout = {
		  .sizing = {outer_size, outer_size},
		  .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER},
		},
		.backgroundColor = hovered ? COLOR_FG_INTER : COLOR_FG,
		.custom = {
		  .customData = knob_element_data,
		},
	}) {
	  CustomElementData *inner_data = arena_alloc(ui_data->arena, sizeof(CustomElementData));
	  inner_data->type = CUSTOM_ELEMENT_TYPE_CIRCLE;
	  inner_data->circle = (CircleData){
		.start_angle = 0,
		.value = 1.0f,
	  };
	  CLAY_AUTO_ID({
		  .layout = {
			.sizing = {inner_size, inner_size},
		  },
		  .backgroundColor = COLOR_ACCENT,
		  .custom = {
			.customData = inner_data,
		  },
	  });
	};
  }
}

void draw_screen(UIData *ui_data) {
  
  CustomElementData *wave = arena_alloc(ui_data->arena, sizeof(CustomElementData));
  wave->type = CUSTOM_ELEMENT_TYPE_WAVE_SCREEN;
  wave->wave_screen = (WaveScreenData){
	.point_buffer = ui_data->wave_buffer,
	.buffer_len = ui_data->wave_buffer_size,
	.thickness = 2,
  };

  CLAY(CLAY_ID("wave_border"), {
    .border = { .width = {1, 1, 1, 1, 0}, .color = COLOR_FG },
  }) {
	CLAY(CLAY_ID("wave_screen"), {
		.layout = {
		  .sizing = {CLAY_SIZING_FIXED(200 * ui_data->scale), CLAY_SIZING_FIXED(100 * ui_data->scale)},
		},
		.backgroundColor = COLOR_FG,
		.custom = {
		  .customData = wave,
		},
		.clip = {
		  .horizontal = true,
		},
	  });
  }
}

void draw_panel(UIData *ui_data) {
  CLAY(CLAY_ID("panel_container"), {
	  .layout = {
		.sizing = {CLAY_SIZING_FIXED(450 * ui_data->scale), CLAY_SIZING_FIXED(100 * ui_data->scale)},
		.childGap = 30,
		.childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER},
	  },
  }) {
	CLAY(CLAY_ID("volume_knob_container")) {
	  draw_knob(CLAY_ID("volume_knob"), ui_data, &ui_data->knob_settings->volume, CLAY_SIZING_FIXED(85 * ui_data->scale), CLAY_SIZING_FIXED(45 * ui_data->scale));
	}

	draw_screen(ui_data);
	
	CLAY(CLAY_ID("envelope_knobs_container"), {
		.layout = {
		  .layoutDirection = CLAY_TOP_TO_BOTTOM,
		  .childGap = 5,
		},
	}) {
	  CLAY(CLAY_ID("envelope_knobs_upper"), {
		  .layout = {
			.childGap = 5,
		  },
	  }) {
		draw_knob(CLAY_ID("attack_knob"), ui_data, &ui_data->knob_settings->attack, CLAY_SIZING_FIXED(40 * ui_data->scale), CLAY_SIZING_FIXED(21 * ui_data->scale));
		draw_knob(CLAY_ID("decay_knob"), ui_data, &ui_data->knob_settings->decay, CLAY_SIZING_FIXED(40 * ui_data->scale), CLAY_SIZING_FIXED(21 * ui_data->scale));
	  }
	  CLAY(CLAY_ID("envelope_knobs_lower"), {
		  .layout = {
			.childGap = 5,
		  },
	  }) {
		draw_knob(CLAY_ID("sustain_knob"), ui_data, &ui_data->knob_settings->sustain, CLAY_SIZING_FIXED(40 * ui_data->scale), CLAY_SIZING_FIXED(21 * ui_data->scale));
		draw_knob(CLAY_ID("release_knob"), ui_data, &ui_data->knob_settings->release, CLAY_SIZING_FIXED(40 * ui_data->scale), CLAY_SIZING_FIXED(21 * ui_data->scale));
	  }
	}
  };
}

void draw_ui(UIData *ui_data) {
  CLAY(CLAY_ID("outer_container"), {
	  .layout = {
		.sizing = {CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0)},
		.childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER},
	  },
	  .backgroundColor = COLOR_BG,
  }) {
	CLAY(CLAY_ID("ui_container"), {
		.layout = {
		  .sizing = {CLAY_SIZING_FIT(0), CLAY_SIZING_FIT(0)},
		  .layoutDirection = CLAY_TOP_TO_BOTTOM,
		  .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER},
		  .childGap = 30,
		},
	}) {
	  draw_panel(ui_data);
	  draw_keyboard(ui_data);
	};
  };
}
