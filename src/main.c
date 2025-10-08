#include <alsa/asoundlib.h>
#include <pthread.h>
#include <stdio.h>

#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_scancode.h>
#include <SDL3/SDL_keycode.h>

#define CLAY_IMPLEMENTATION
#include "clay/clay.h"
#include "clay/renderers/clay_renderer_SDL3.c"

#include "sounds.h"

static const int SCREEN_FPS = 60;
static const int SCREEN_TICKS_PER_FRAME = 1000 / SCREEN_FPS;

static const int FONT_ID = 0;

static const Clay_Color COLOR_BG = (Clay_Color){45, 53, 59, 255};
static const Clay_Color COLOR_FG = (Clay_Color){211, 198, 170, 255};

typedef struct {
  char key;
  float freq;
} freq_map;

typedef struct {
  SDL_Window *window;
  Clay_SDL3RendererData renderer_data;
  
  bool last_keys[12];
  
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

  synth_message param_message = {
      .type = MSG_PARAM_CHANGE,
      .param_change =
          {
              .param_type = PARAM_OSC,
              .value = OSC_SQUARE,
          },
  };
  mqueue_push(&state->msg_queue, param_message);
  param_message = (synth_message){
      .type = MSG_PARAM_CHANGE,
      .param_change =
          {
              .param_type = PARAM_VOLUME,
              .value = 1,
          },
  };
  mqueue_push(&state->msg_queue, param_message);
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

  if (!SDL_CreateWindowAndRenderer("crynth", 640, 480, SDL_WINDOW_RESIZABLE | SDL_WINDOW_BORDERLESS, &state->window, &state->renderer_data.renderer)) {
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
  Clay_Initialize(clayMemory, (Clay_Dimensions) { (float) width, (float) height }, (Clay_ErrorHandler) { HandleClayErrors });
  Clay_SetMeasureTextFunction(SDL_MeasureText, state->renderer_data.fonts);
  
  return 0;
}

SDL_AppResult SDL_AppInit(void **appstate, int argc, char **argv) {
  (void) argc;
  (void) argv;
  
  app_state *state = malloc(sizeof(app_state));
  *appstate = state;

  if (init_ui(state) != 0) {
	printf("Couldn't initialize UI: %s", SDL_GetError());
	return SDL_APP_FAILURE;
  }

  if (init_sounds(state) != 0) {
    return SDL_APP_FAILURE;
  }

  return SDL_APP_CONTINUE;
}

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

SDL_AppResult SDL_AppIterate(void *appstate) {
  app_state *state = appstate;

  int start_tick = SDL_GetTicks();

  Clay_BeginLayout();

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
    draw_keyboard(state->last_keys, 12);
  };
  
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

SDL_Keycode keys[12] = {
  SDL_SCANCODE_Z, SDL_SCANCODE_S,
  SDL_SCANCODE_X, SDL_SCANCODE_D,
  SDL_SCANCODE_C,
  SDL_SCANCODE_V, SDL_SCANCODE_G,
  SDL_SCANCODE_B, SDL_SCANCODE_H,
  SDL_SCANCODE_N, SDL_SCANCODE_J,
  SDL_SCANCODE_M,
};

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event) {
  app_state *state = appstate;

  switch (event->type) {
  case SDL_EVENT_QUIT: {
    return SDL_APP_SUCCESS;
  }
  case SDL_EVENT_KEY_DOWN: {
	for (int i = 0; i < 12; i++) {
	  if (event->key.scancode == keys[i]) {
		if (state->last_keys[i]) {
		  break;
		}
		state->last_keys[i] = true;
		
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
	  if (event->key.scancode == keys[i]) {
		if (!state->last_keys[i]) {
		  break;
		}
		state->last_keys[i] = false;
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
    Clay_SetPointerState((Clay_Vector2) { event->motion.x, event->motion.y },
                         event->motion.state & SDL_BUTTON_LMASK);
    break;

  case SDL_EVENT_MOUSE_BUTTON_DOWN:
    Clay_SetPointerState((Clay_Vector2) { event->button.x, event->button.y },
                         event->button.button == SDL_BUTTON_LEFT);
    break;

  case SDL_EVENT_MOUSE_WHEEL:
    Clay_UpdateScrollContainers(true, (Clay_Vector2) { event->wheel.x, event->wheel.y }, 0.01f);
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
