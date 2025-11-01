#ifndef MESSAGE_QUEUE_H_
#define MESSAGE_QUEUE_H_

#include <stddef.h>
#include <pthread.h>
#include <stdatomic.h>
#include "defines.h"

typedef enum {
  PARAM_OSC,
  PARAM_VOLUME,

  PARAM_ATTACK,
  PARAM_DECAY,
  PARAM_SUSTAIN,
  PARAM_RELEASE,
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
  float freq;
  float buffers[2][DISPLAY_SAMPLES];
  atomic_int write_index;
} WaveData;

int mqueue_get(message_queue *q, synth_message *msg);
int mqueue_push(message_queue *q, synth_message msg);
int mqueue_push_many(message_queue *q, synth_message *msg, size_t count);

#define mqueue_init(q)                                                         \
  do {                                                                         \
	(q)->head = (q)->tail = 0;                                                 \
	pthread_mutex_init(&(q)->lock, NULL);                                      \
  } while (0)

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

#endif // MESSAGE_QUEUE_H_
