#ifndef SOUNDS_H_
#define SOUNDS_H_

#include <alsa/asoundlib.h>
#include <math.h>
#include <pthread.h>
#include <stdbool.h>
#include <limits.h>

#define check(ret)                                                          \
	do {                                                                    \
		int res = (ret);                                                    \
		if (res < 0) {                                                      \
			fprintf(stderr, "%s:%d ERROR: %s (%d)\n",                       \
				__FILE__, __LINE__, snd_strerror(res), res);                \
			exit(1);                                                        \
		}                                                                   \
	} while (0)

#define MESSAGE_QUEUE_SIZE 128
#define SAMPLE_RATE 48000
#define PERIOD_SIZE 480

typedef enum {
  PARAM_OSC,
  PARAM_VOLUME,
} param_type;

typedef enum {
  OSC_SINE,
  OSC_SAW,
  OSC_SQUARE,
  OSC_TRIANGLE,
} oscilator_type;

typedef enum {
  MSG_NOTE_ON,
  MSG_NOTE_OFF,
  MSG_ALL_NOTES_OFF,
  MSG_PARAM_CHANGE,
  MSG_STOP,
} synth_message_type;

typedef struct {
  synth_message_type type;

  union {
	// NOTE_ON / NOTE_OFF
	struct {
	  size_t note_id;
	} note;

	// SET_PARAM;
	struct {
	  param_type param_type;
	  float value;
	} param_change;
  };
} synth_message;


typedef struct {
  synth_message buffer[MESSAGE_QUEUE_SIZE];
  size_t head;
  size_t tail;
  pthread_mutex_t lock;
} message_queue;

typedef struct {
  snd_pcm_t *pcm;
  message_queue *queue;
} sound_thread_meta;

typedef struct {
  bool active;
  float freq;
  float phase;
  float phase_inc;
} synth_voice;

typedef struct {
  synth_voice *buffer;
  size_t size;
} synth_voices;

typedef struct {
  oscilator_type oscilator_type;
  float master_volume;
} synth_params;

typedef void (*oscilator_func)(float *buffer, size_t sample_count, float *phase, float phase_inc);

#define mqueue_init(q) \
  do { \
    (q)->head = (q)->tail = 0; \
    pthread_mutex_init(&(q)->lock, NULL); \
  } while (0)


#define mqueue_push(q, msg) \
  do { \
	pthread_mutex_lock(&(q)->lock); \
	size_t next = ((q)->head + 1) % MESSAGE_QUEUE_SIZE; \
	 \
	if ((q)->tail == next) { \
	  pthread_mutex_unlock(&(q)->lock); \
	  return 1; \
	} \
	 \
	(q)->buffer[(q)->head] = msg; \
	(q)->head = next; \
	 \
	pthread_mutex_unlock(&(q)->lock); \
  } while (0) \

/* #define mqueue_get(q, msg, ok) \ */
/*   do { \ */
/*     pthread_mutex_lock(&(q)->lock); \ */
/*     if ((q)->tail == (q)->head) { \ */
/*       pthread_mutex_unlock(&(q)->lock); \ */
/*       *(ok) = false;\ */
/*   	  break; \ */
/*     } \ */
/*    \ */
/*     *(msg) = (q)->buffer[(q)->tail]; \ */
/*     (q)->tail = ((q)->tail + 1) % MESSAGE_QUEUE_SIZE; \ */
/*    \ */
/*     pthread_mutex_unlock(&(q)->lock); \ */
/*     *(ok) = true; \ */
/*   } while (0) */

#define mqueue_empty(q) (q)->head == (q)->tail

void *sound_thread_start(void *ptr);
int set_hw_params(snd_pcm_t *pcm);

#endif // SOUNDS_H_
