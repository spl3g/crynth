#include <alsa/asoundlib.h>
#include <pthread.h>
#include <stdio.h>

#include "ui.h"
#include "sounds.h"
#include "custom_elements.h"

#define SOKOL_IMPL
#define SOKOL_GLCORE
#include "sokol/sokol_gfx.h"
#include "sokol/sokol_gl.h"
#include "sokol/sokol_glue.h"
#include "sokol/sokol_app.h"
#include "sokol/sokol_log.h"

#define FONTSTASH_IMPLEMENTATION
#include "fontstash/fontstash.h"
#include "sokol/sokol_fontstash.h"

#define CLAY_IMPLEMENTATION
#include "clay.h"

#define SOKOL_CLAY_IMPL
#include "sokol/sokol_clay.h"

#define ARENA_IMPLEMENTATION
#include "arena.h"


typedef struct {
  bool pressed;

  float position_x;
  float position_y;
  float pending_scroll_delta_x;
  float pending_scroll_delta_y;
} PointerState;


typedef struct {
  Arena frame_arena;
  PointerState pointer;
  
  KeyState *keys;
  size_t keys_amount;

  KnobSettings knob_settings;
  
  snd_pcm_t *sound_device;
  MessageQueue msg_queue;
  WaveData wave_data;
  pthread_t sound_thread;
} AppState;

int init_sounds(AppState *state) {
  int err;

  err =
    snd_pcm_open(&state->sound_device, "default", SND_PCM_STREAM_PLAYBACK, 0);
  if (err < 0) {
    printf("error: failed to open device: %s", snd_strerror(err));
    return err;
  }

  err = set_hw_params(state->sound_device);
  if (err < 0) {
    printf("error: failed to set parameters: %s", snd_strerror(err));
    return err;
  }

  mqueue_init(&state->msg_queue);

  SoundThreadMeta *sound_thread_params = malloc(sizeof(SoundThreadMeta));
  sound_thread_params->pcm = state->sound_device;
  sound_thread_params->queue = &state->msg_queue;
  sound_thread_params->wave_data = &state->wave_data;

  pthread_t sound_thread;
  pthread_create(&sound_thread, NULL, sound_thread_start, sound_thread_params);

  state->sound_thread = sound_thread;
  state->knob_settings = (KnobSettings){
	.volume = {
	  .param_type = PARAM_VOLUME,
	  .value = 0.2,
	  .range_start = 0,
	  .range_end = 1,
	},
	.attack = {
	  .param_type = PARAM_ATTACK,
	  .value = 5,
	  .range_start = 0,
	  .range_end = 1000,
	},
	.decay = {
	  .param_type = PARAM_DECAY,
	  .value = 10,
	  .range_start = 0,
	  .range_end = 1000,
	},
	.sustain = {
	  .param_type = PARAM_SUSTAIN,
	  .value = 0.7,
	  .range_start = 0,
	  .range_end = 1,
	},
	.release = {
	  .param_type = PARAM_RELEASE,
	  .value = 100,
	  .range_start = 0,
	  .range_end = 5000,
	},
  };

  SynthMessage param_messages[6] = {
	{
      .type = MSG_PARAM_CHANGE,
      .param_change =
	  {
		.param_type = PARAM_OSC,
		.value = OSC_SAW,
	  },
	},
	{
      .type = MSG_PARAM_CHANGE,
      .param_change =
	  {
		.param_type = PARAM_VOLUME,
		.value = state->knob_settings.volume.value,
	  },
	},
	{
	  .type = MSG_PARAM_CHANGE,
	  .param_change =
	  {
		.param_type = PARAM_ATTACK,
		.value = state->knob_settings.attack.value,
	  },
	},
	{
	  .type = MSG_PARAM_CHANGE,
	  .param_change =
	  {
		.param_type = PARAM_DECAY,
		.value = state->knob_settings.decay.value,
	  },
	},
	{
	  .type = MSG_PARAM_CHANGE,
	  .param_change =
	  {
		.param_type = PARAM_SUSTAIN,
		.value = state->knob_settings.sustain.value,
	  },
	},
	{
	  .type = MSG_PARAM_CHANGE,
	  .param_change =
	  {
		.param_type = PARAM_RELEASE,
		.value = state->knob_settings.release.value,
	  },
	},
  };
  mqueue_push_many(&state->msg_queue, param_messages, 6);
  return 0;
}

void HandleClayErrors(Clay_ErrorData errorData) {
  printf("%s", errorData.errorText.chars);
}

int init_ui() {
  sg_setup(&(sg_desc){
    .environment = sglue_environment(),
    .logger.func = slog_func,
  });
  sgl_setup(&(sgl_desc_t){
    .logger.func = slog_func,
  });
  sclay_setup();

  sclay_add_font("resources/Roboto-Regular.ttf");
  sclay_set_custom_element_cb(handle_custom);

  size_t totalMemorySize = Clay_MinMemorySize();
  Clay_Arena clayMemory = (Clay_Arena) {
    .memory = malloc(totalMemorySize),
    .capacity = totalMemorySize
  };

  Clay_Initialize(clayMemory, (Clay_Dimensions) { (float) sapp_width(), (float) sapp_height() }, (Clay_ErrorHandler) { HandleClayErrors, 0});
  Clay_SetMeasureTextFunction(sclay_measure_text, NULL);
  
  return 0;
}

static KeyState keys[] = {
  {'Z', SAPP_KEYCODE_Z, 0, 0}, {'S', SAPP_KEYCODE_S, 0, 0},
  {'X', SAPP_KEYCODE_X, 0, 0}, {'D', SAPP_KEYCODE_D, 0, 0},
  {'C', SAPP_KEYCODE_C, 0, 0},
  {'V', SAPP_KEYCODE_V, 0, 0}, {'G', SAPP_KEYCODE_G, 0, 0},
  {'B', SAPP_KEYCODE_B, 0, 0}, {'H', SAPP_KEYCODE_H, 0, 0},
  {'N', SAPP_KEYCODE_N, 0, 0}, {'J', SAPP_KEYCODE_J, 0, 0},
  {'M', SAPP_KEYCODE_M, 0, 0},

  {'Q', SAPP_KEYCODE_Q, 0, 0}, {'2', SAPP_KEYCODE_2, 0, 0},
  {'W', SAPP_KEYCODE_W, 0, 0}, {'3', SAPP_KEYCODE_3, 0, 0},
  {'E', SAPP_KEYCODE_E, 0, 0},
  {'R', SAPP_KEYCODE_R, 0, 0}, {'6', SAPP_KEYCODE_5, 0, 0},
  {'T', SAPP_KEYCODE_T, 0, 0}, {'7', SAPP_KEYCODE_6, 0, 0},
  {'Y', SAPP_KEYCODE_Y, 0, 0}, {'8', SAPP_KEYCODE_7, 0, 0},
  {'U', SAPP_KEYCODE_U, 0, 0},
};

static void init(void *app_state) {
  AppState *state = app_state;
  state->keys = keys;
  state->keys_amount = sizeof(keys)/sizeof(keys[0]);

  if (init_ui() != 0) {
	printf("Couldn't initialize UI");
	sapp_quit();
  }

  if (init_sounds(state) != 0) {
	printf("Couldn't initialize sounds: %s");
    sapp_quit();
  }
}

float old_wave_buffer[DISPLAY_SAMPLES];

void fill_ui_data(UIData *ui_data, AppState *state) {
  Clay_Dimensions dimensions = Clay_GetCurrentContext()->layoutDimensions;

  ui_data->arena = &state->frame_arena;
  ui_data->msg_queue = &state->msg_queue;

  int read_index = 1 - atomic_load(&state->wave_data.write_index);
  float *wave_buffer = state->wave_data.buffers[read_index];

  size_t buffer_start = 0;
  for (size_t i = 1; i < DISPLAY_SAMPLES; i++) {
	if (wave_buffer[i-1] < 0 && wave_buffer[i] >= 0) {
	  buffer_start = i;
	  break;
	}
  }

  if (buffer_start < 100) {
	ui_data->wave_buffer = &wave_buffer[buffer_start];
	ui_data->wave_buffer_size = DISPLAY_SAMPLES - 100;
	memcpy(old_wave_buffer, &wave_buffer[buffer_start], (DISPLAY_SAMPLES - 100) * sizeof(float));
  } else {
	ui_data->wave_buffer = old_wave_buffer;
	ui_data->wave_buffer_size = DISPLAY_SAMPLES - 100;
  }

  ui_data->keys = state->keys;
  ui_data->keys_amount = state->keys_amount;

  ui_data->knob_settings = &state->knob_settings;
  ui_data->scale = dimensions.width / DEFAULT_DIMENSIONS_WIDTH;  
}

static void frame(void *app_state) {
  AppState *state = app_state;
  
  Arena *arena = &state->frame_arena;
  arena_reset(arena);
  UIData *ui_data = arena_alloc(arena, sizeof(UIData));
  fill_ui_data(ui_data, state);


  sclay_new_frame();
  Clay_BeginLayout();

  draw_ui(ui_data);

  Clay_RenderCommandArray render_commands = Clay_EndLayout();


  sg_begin_pass(&(sg_pass){ .swapchain = sglue_swapchain() });
  
  sgl_matrix_mode_modelview();
  sgl_load_identity();

  sclay_render(render_commands, NULL);

  sgl_draw();
  sg_end_pass();
  sg_commit();

  /* int end_tick = SDL_GetTicks(); */
  /* int frame_ticks = end_tick - start_tick; */

  /* if (frame_ticks < SCREEN_TICKS_PER_FRAME) { */
  /* 	SDL_Delay(SCREEN_TICKS_PER_FRAME - frame_ticks); */
  /* } */
}

static void event(const sapp_event* event, void *app_state) {
  AppState *state = app_state;

  switch (event->type) {
  case SAPP_EVENTTYPE_KEY_DOWN: {
	for (size_t i = 0; i < state->keys_amount; i++) {
	  if (event->key_code == state->keys[i].keycode) {
		if (state->keys[i].keyboard_pressed) {
		  break;
		}
		state->keys[i].keyboard_pressed = true;
		
		mqueue_push(&state->msg_queue, (SynthMessage){
		  .type = MSG_NOTE_ON,
		  .note = { .note_id = i },
		});
	  }
	}
	break;
  }

  case SAPP_EVENTTYPE_KEY_UP: {
	for (size_t i = 0; i < state->keys_amount; i++) {
	  if (event->key_code == state->keys[i].keycode) {
		if (!state->keys[i].keyboard_pressed) {
		  break;
		}
		state->keys[i].keyboard_pressed = false;
		mqueue_push(&state->msg_queue, (SynthMessage){
		  .type = MSG_NOTE_OFF,
		  .note = { .note_id = i },
		});
	  }
	}
	break;
  }

  default:
	sclay_handle_event(event);
	break;
  }
}

static void cleanup() {
  sclay_shutdown();
  sgl_shutdown();
  sg_shutdown();
}

sapp_desc sokol_main(int argc, char **argv) {
  (void)argc;
  (void)argv;

  AppState *state = malloc(sizeof(AppState));
  
  return (sapp_desc){
	.user_data = state,
    .init_userdata_cb = init,
    .frame_userdata_cb = frame,
    .event_userdata_cb = event,
    .cleanup_cb = cleanup,
    .window_title = "crynth",
    .width = DEFAULT_DIMENSIONS_WIDTH,
    .height = DEFAULT_DIMENSIONS_HEIGHT,
    .logger.func = slog_func,
  };
}
