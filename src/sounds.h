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

typedef enum {
  ENV_OFF,
  ENV_ATTACK,
  ENV_DECAY,
  ENV_SUSTAIN,
  ENV_RELEASE,
} EnvelopeState;

typedef struct {
  int attack_time;
  int decay_time;
  float sustain_level;
  int release_time;
} EnvelopeParams;

typedef struct {
  EnvelopeState state;
  int counter;
  float current_inc;
  float release_value;
  EnvelopeParams params;
} Envelope;

typedef struct {
  bool active;
  float freq;
  float phase;
  float phase_inc;
  Envelope envelope;
} SynthVoice;

typedef struct {
  SynthVoice *buffer;
  size_t size;
} SynthVoices;

typedef struct {
  OscilatorType oscilator_type;
  float master_volume;
  EnvelopeParams envelope_params;
  float last_freq;
} SynthParams;

typedef float (*OscilatorFunc)(float phase);

void *sound_thread_start(void *ptr);
int set_hw_params(snd_pcm_t *pcm);

#endif // SOUNDS_H_
