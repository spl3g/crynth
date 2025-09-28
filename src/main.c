#include <stdio.h>
#include "sounds.h"
#include <alsa/asoundlib.h>
#include <raylib.h>
#include <pthread.h>
#include <stdbool.h>

typedef struct {
  char key;
  float freq;
} freq_map;

int main() {
  snd_pcm_t *sound_device;
  int err;

  err = snd_pcm_open(&sound_device, "default", SND_PCM_STREAM_PLAYBACK, 0);
  if (err < 0) {
	printf("error: failed to open device: %s", snd_strerror(err));
	return 1;
  }

  err = set_hw_params(sound_device);
  if (err < 0) {
	printf("error: failed to set parameters: %s", snd_strerror(err));
	return 1;
  }

  SetConfigFlags(FLAG_WINDOW_RESIZABLE);
  InitWindow(800, 800, "crynth");
  SetTargetFPS(60);

  message_queue queue;
  mqueue_init(&queue);

  sound_thread_meta sound_thread_params = {
	.pcm = sound_device,
	.queue = &queue,
  };

  pthread_t sound_thread;
  pthread_create(&sound_thread, NULL, sound_thread_start, &sound_thread_params);

  /* synth_message param_message = { */
  /* 	.type = MSG_PARAM_CHANGE, */
  /* 	.param_change = { */
  /* 	  .param_type = PARAM_OSC, */
  /* 	  .value = OSC_SINE, */
  /* 	}, */
  /* }; */
  /* mqueue_push(&queue, param_message); */
  /* param_message = (synth_message){ */
  /* 	.type = MSG_PARAM_CHANGE, */
  /* 	.param_change = { */
  /* 	  .param_type = PARAM_VOLUME, */
  /* 	  .value = 0.2, */
  /* 	}, */
  /* }; */
  /* mqueue_push(&queue, param_message); */

  int keys[12] = {
	KEY_Z,
	KEY_S,
	KEY_X,
	KEY_D,
	KEY_C,
	KEY_V,
	KEY_G,
	KEY_B,
	KEY_H,
	KEY_N,
	KEY_J,
	KEY_M,
  };

  while (!WindowShouldClose()) {
	for (int i = 0; i < 12; i++) {
	  if (IsKeyPressed(keys[i])) {
		synth_message message = {
		  .type = MSG_NOTE_ON,
		  .note = {
			.note_id = i,
		  },
		};
		mqueue_push(&queue, message);
	  }
	
	  if (IsKeyReleased(keys[i])) {
		synth_message message = {
		  .type = MSG_NOTE_OFF,
		  .note = {
			.note_id = i,
		  },
		};
		mqueue_push(&queue, message);
	  }
	}

	BeginDrawing();
	ClearBackground(RAYWHITE);
	EndDrawing();
  }

  synth_message stop_msg = {.type = MSG_STOP};
  mqueue_push(&queue, stop_msg);
  CloseWindow();

  pthread_join(sound_thread, NULL);
  check(snd_pcm_close(sound_device));

  return 0;
}
