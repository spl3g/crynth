#ifndef SOUNDS_H_
#define SOUNDS_H_

#include <alsa/asoundlib.h>
#include <limits.h>
#include <math.h>
#include <pthread.h>
#include <stdbool.h>

#include "messages.h"

#define check(ret)                                                             \
  do {                                                                         \
    int res = (ret);                                                           \
    if (res < 0) {                                                             \
      fprintf(stderr, "%s:%d ERROR: %s (%d)\n", __FILE__, __LINE__,            \
              snd_strerror(res), res);                                         \
      exit(1);                                                                 \
    }                                                                          \
  } while (0)

#define SAMPLE_RATE 48000
#define PERIOD_SIZE 480

typedef struct {
  snd_pcm_t *pcm;
  message_queue *queue;
} sound_thread_meta;

typedef enum {
  ENV_OFF,
  ENV_ATTACK,
  ENV_DECAY,
  ENV_SUSTAIN,
  ENV_RELEASE,
} envelope_state;

typedef struct {
  int attack_time;
  int decay_time;
  float sustain_level;
  int release_time;
} envelope_params;

typedef struct {
  envelope_state state;
  int counter;
  float current_inc;
  float release_value;
  envelope_params params;
} envelope;

typedef struct {
  bool active;
  float freq;
  float phase;
  float phase_inc;
  envelope envelope;
} synth_voice;

typedef struct {
  synth_voice *buffer;
  size_t size;
} synth_voices;

typedef struct {
  oscilator_type oscilator_type;
  float master_volume;
  envelope_params envelope_params;
} synth_params;

typedef float (*oscilator_func)(float phase);

void *sound_thread_start(void *ptr);
int set_hw_params(snd_pcm_t *pcm);

#endif // SOUNDS_H_
