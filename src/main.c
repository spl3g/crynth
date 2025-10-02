#include <alsa/asoundlib.h>
#include <pthread.h>
#include <stdio.h>

#include "clay/clay.h"

#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_scancode.h>
#include <SDL3/SDL_keycode.h>

#include "sounds.h"

const int SCREEN_FPS = 60;
const int SCREEN_TICKS_PER_FRAME = 1000 / SCREEN_FPS;

typedef struct {
  char key;
  float freq;
} freq_map;

typedef struct {
  SDL_Window *window;
  SDL_Renderer *renderer;
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

SDL_AppResult SDL_AppInit(void **appstate, int argc, char **argv) {
  app_state *state = malloc(sizeof(app_state));
  *appstate = state;

  if (!SDL_Init(SDL_INIT_VIDEO)) {
    printf("Couldn't initialize SDL: %s", SDL_GetError());
    return SDL_APP_FAILURE;
  }

  if (!SDL_CreateWindowAndRenderer("crynth", 640, 480, 0, &state->window,
                                   &state->renderer)) {
    printf("Couldn't create window/renderer: %s", SDL_GetError());
    return SDL_APP_FAILURE;
  }

  if (init_sounds(state) != 0) {
    return SDL_APP_FAILURE;
  }

  return SDL_APP_CONTINUE;
}


SDL_AppResult SDL_AppIterate(void *appstate) {
  app_state *state = appstate;

  int start_tick = SDL_GetTicks();
	
  const double now = ((double)SDL_GetTicks()) / 1000.0;
  const float red = (float)(0.5 + 0.5 * SDL_sin(now));
  const float green = (float)(0.5 + 0.5 * SDL_sin(now + SDL_PI_D * 2 / 3));
  const float blue = (float)(0.5 + 0.5 * SDL_sin(now + SDL_PI_D * 4 / 3));
  SDL_SetRenderDrawColorFloat(state->renderer, red, green, blue, SDL_ALPHA_OPAQUE_FLOAT);

  SDL_RenderClear(state->renderer);

  SDL_RenderPresent(state->renderer);

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

  default: {};
  }
  return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result) {
  app_state *state = appstate;

  message_queue *queue = &state->msg_queue;

  synth_message stop_msg = {.type = MSG_STOP};
  mqueue_push(queue, stop_msg);

  pthread_join(state->sound_thread, NULL);
  check(snd_pcm_close(state->sound_device));
}
