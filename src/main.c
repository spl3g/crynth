#include <alsa/asoundlib.h>
#include <pthread.h>
#include <stdio.h>

#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_scancode.h>
#include <SDL3/SDL_keycode.h>

#include "ui.h"
#include "sounds.h"

#define CLAY_IMPLEMENTATION
#include "clay.h"
#include "clay_renderer_SDL3.h"

#define ARENA_IMPLEMENTATION
#include "arena.h"

static const int SCREEN_FPS = 60;
static const int SCREEN_TICKS_PER_FRAME = 1000 / SCREEN_FPS;

static const int FONT_ID = 0;
static const Clay_Dimensions DEFAULT_DIMENSIONS = {
  .width = 1280,
  .height = 720,
};

typedef struct {
  bool pressed;

  float position_x;
  float position_y;
  float pending_scroll_delta_x;
  float pending_scroll_delta_y;
} PointerState;


typedef struct {
  SDL_Window *window;
  Clay_SDL3RendererData renderer_data;
  Arena frame_arena;
  PointerState pointer;
  
  KeyState *keys;
  size_t keys_amount;

  KnobSettings knob_settings;
  
  snd_pcm_t *sound_device;
  message_queue msg_queue;
  pthread_t sound_thread;
} app_state;

int init_sounds(app_state *state) {
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

  sound_thread_meta *sound_thread_params = malloc(sizeof(sound_thread_meta));
  sound_thread_params->pcm = state->sound_device;
  sound_thread_params->queue = &state->msg_queue;

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

  synth_message param_messages[6] = {
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

static inline Clay_Dimensions SDL_MeasureText(Clay_StringSlice text, Clay_TextElementConfig *config, void *userData)
{
  TTF_Font **fonts = userData;
  TTF_Font *font = fonts[config->fontId];
  int width, height;

  TTF_SetFontSize(font, config->fontSize);
  if (!TTF_GetStringSize(font, text.chars, text.length, &width, &height)) {
    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to measure text: %s", SDL_GetError());
  }

  return (Clay_Dimensions) { (float) width, (float) height };
}

void HandleClayErrors(Clay_ErrorData errorData) {
  printf("%s", errorData.errorText.chars);
}

int init_ui(app_state *state) {
  if (!TTF_Init()) {
	return 1;
  }

  if (!SDL_Init(SDL_INIT_VIDEO)) {
	return 1;
  }

  if (!SDL_CreateWindowAndRenderer("crynth", DEFAULT_DIMENSIONS.width, DEFAULT_DIMENSIONS.height, SDL_WINDOW_RESIZABLE | SDL_WINDOW_BORDERLESS, &state->window, &state->renderer_data.renderer)) {
	return 1;
  }

  SDL_SetWindowResizable(state->window, true);

  state->renderer_data.textEngine = TTF_CreateRendererTextEngine(state->renderer_data.renderer);
  if (!state->renderer_data.textEngine) {
	return 1;
  }

  state->renderer_data.fonts = SDL_calloc(1, sizeof(TTF_Font *));
  if (!state->renderer_data.fonts) {
	return 1;
  }

  TTF_Font *font = TTF_OpenFont("resources/Roboto-Regular.ttf", 24);
  if (!font) {
	return 1;
  }

  state->renderer_data.fonts[FONT_ID] = font;

  size_t totalMemorySize = Clay_MinMemorySize();
  Clay_Arena clayMemory = (Clay_Arena) {
    .memory = SDL_malloc(totalMemorySize),
    .capacity = totalMemorySize
  };

  int width, height;
  SDL_GetWindowSize(state->window, &width, &height);
  Clay_Initialize(clayMemory, (Clay_Dimensions) { (float) width, (float) height }, (Clay_ErrorHandler) { HandleClayErrors, 0});
  Clay_SetMeasureTextFunction(SDL_MeasureText, state->renderer_data.fonts);
  
  return 0;
}

KeyState keys[12] = {
  {'Z', SDL_SCANCODE_Z, 0, 0}, {'S', SDL_SCANCODE_S, 0, 0},
  {'X', SDL_SCANCODE_X, 0, 0}, {'D', SDL_SCANCODE_D, 0, 0},
  {'C', SDL_SCANCODE_C, 0, 0},
  {'V', SDL_SCANCODE_V, 0, 0}, {'G', SDL_SCANCODE_G, 0, 0},
  {'B', SDL_SCANCODE_B, 0, 0}, {'H', SDL_SCANCODE_H, 0, 0},
  {'N', SDL_SCANCODE_N, 0, 0}, {'J', SDL_SCANCODE_J, 0, 0},
  {'M', SDL_SCANCODE_M, 0, 0},
};

SDL_AppResult SDL_AppInit(void **appstate, int argc, char **argv) {
  (void) argc;
  (void) argv;

  app_state *state = malloc(sizeof(app_state));
  memset(state, 0, sizeof(app_state));
  *appstate = state;
  state->keys = keys;
  state->keys_amount = 12;

  if (init_ui(state) != 0) {
	printf("Couldn't initialize UI: %s", SDL_GetError());
	return SDL_APP_FAILURE;
  }

  if (init_sounds(state) != 0) {
    return SDL_APP_FAILURE;
  }

  return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *appstate) {
  app_state *state = appstate;
  Arena *arena = &state->frame_arena;
  arena_reset(arena);

  int start_tick = SDL_GetTicks();

  Clay_Dimensions dimensions = Clay_GetCurrentContext()->layoutDimensions;

  UIData *ui_data = arena_alloc(arena, sizeof(UIData));
  ui_data->msg_queue = &state->msg_queue;
  ui_data->keys = state->keys;
  ui_data->keys_amount = 12;
  ui_data->arena = arena;
  ui_data->knob_settings = &state->knob_settings;
  ui_data->scale = dimensions.width / DEFAULT_DIMENSIONS.width;

  Clay_SetPointerState((Clay_Vector2){ state->pointer.position_x, state->pointer.position_y }, state->pointer.pressed);
  Clay_UpdateScrollContainers(true, (Clay_Vector2){ state->pointer.pending_scroll_delta_x, state->pointer.pending_scroll_delta_y }, 0.016f);
  state->pointer.pending_scroll_delta_x = 0;
  state->pointer.pending_scroll_delta_y = 0;
  
  Clay_BeginLayout();

  draw_ui(ui_data);

  Clay_RenderCommandArray render_commands = Clay_EndLayout();

  SDL_SetRenderDrawColor(state->renderer_data.renderer, 0, 0, 0, 255);
  SDL_RenderClear(state->renderer_data.renderer);

  SDL_Clay_RenderClayCommands(&state->renderer_data, &render_commands);

  SDL_RenderPresent(state->renderer_data.renderer);

  int end_tick = SDL_GetTicks();
  int frame_ticks = end_tick - start_tick;

  if (frame_ticks < SCREEN_TICKS_PER_FRAME) {
	SDL_Delay(SCREEN_TICKS_PER_FRAME - frame_ticks);
  }

  return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event) {
  app_state *state = appstate;

  switch (event->type) {
  case SDL_EVENT_QUIT: {
    return SDL_APP_SUCCESS;
  }
  case SDL_EVENT_KEY_DOWN: {
	for (int i = 0; i < 12; i++) {
	  if (event->key.scancode == state->keys[i].keycode) {
		if (state->keys[i].keyboard_pressed) {
		  break;
		}
		state->keys[i].keyboard_pressed = true;
		
		mqueue_push(&state->msg_queue, (synth_message){
		  .type = MSG_NOTE_ON,
		  .note = { .note_id = i },
		});
	  }
	}
	break;
  }

  case SDL_EVENT_KEY_UP: {
	for (int i = 0; i < 12; i++) {
	  if (event->key.scancode == state->keys[i].keycode) {
		if (!state->keys[i].keyboard_pressed) {
		  break;
		}
		state->keys[i].keyboard_pressed = false;
		mqueue_push(&state->msg_queue, (synth_message){
		  .type = MSG_NOTE_OFF,
		  .note = { .note_id = i },
		});
	  }
	}
	break;
  }

  case SDL_EVENT_WINDOW_RESIZED:
	Clay_SetLayoutDimensions((Clay_Dimensions) { (float) event->window.data1, (float) event->window.data2 });
    break;

  case SDL_EVENT_MOUSE_MOTION:
	state->pointer.pressed = event->motion.state & SDL_BUTTON_LMASK;
	state->pointer.position_x = event->motion.x;
	state->pointer.position_y = event->motion.y;
    break;

  case SDL_EVENT_MOUSE_BUTTON_DOWN:
	state->pointer.pressed = event->button.button == SDL_BUTTON_LEFT;
	state->pointer.position_x = event->button.x;
	state->pointer.position_y = event->button.y;
    break;
  case SDL_EVENT_MOUSE_BUTTON_UP:
	if (event->button.button == SDL_BUTTON_LEFT) {
	  state->pointer.pressed = false;
	  state->pointer.position_x = event->button.x;
	  state->pointer.position_y = event->button.y;
	}
	break;

  case SDL_EVENT_MOUSE_WHEEL:
	state->pointer.pending_scroll_delta_x += event->wheel.x;
	state->pointer.pending_scroll_delta_y += event->wheel.y;
    break;

  default:
	break;
  }
  return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result) {
  (void) result;
  
  app_state *state = appstate;

  message_queue *queue = &state->msg_queue;

  synth_message stop_msg = {.type = MSG_STOP};
  mqueue_push(queue, stop_msg);

  pthread_join(state->sound_thread, NULL);
  check(snd_pcm_close(state->sound_device));
}
