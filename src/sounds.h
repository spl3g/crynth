#ifndef SOUNDS_H_
#define SOUNDS_H_

#include <alsa/asoundlib.h>
#include <limits.h>
#include <math.h>
#include <pthread.h>
#include <stdbool.h>

#include "messages.h"
#include "midi_freqs.h"
#include "defines.h"

#define check(ret)                                                             \
  do {                                                                         \
    int res = (ret);                                                           \
    if (res < 0) {                                                             \
      fprintf(stderr, "%s:%d ERROR: %s (%d)\n", __FILE__, __LINE__,            \
              snd_strerror(res), res);                                         \
      exit(1);                                                                 \
    }                                                                          \
  } while (0)

typedef struct {
  snd_pcm_t *pcm;
  MessageQueue *queue;
  WaveData *wave_data;
} SoundThreadMeta;

void *sound_thread_start(void *ptr);
int set_hw_params(snd_pcm_t *pcm);

#endif // SOUNDS_H_
