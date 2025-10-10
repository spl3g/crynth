#include "messages.h"

int mqueue_get(message_queue *q, synth_message *msg) {
  pthread_mutex_lock(&(q)->lock);
  if ((q)->tail == (q)->head) {
	pthread_mutex_unlock(&(q)->lock);
	return 1;
  }

  *(msg) = (q)->buffer[(q)->tail];
  (q)->tail = ((q)->tail + 1) % MESSAGE_QUEUE_SIZE;

  pthread_mutex_unlock(&(q)->lock);
  return 0;
}

int mqueue__push_no_lock(message_queue *q, synth_message msg) {
  size_t next = ((q)->head + 1) % MESSAGE_QUEUE_SIZE;

  if ((q)->tail == next) {
	return 1;
  }

  (q)->buffer[(q)->head] = msg;
  (q)->head = next;
  return 0;
}

int mqueue_push(message_queue *q, synth_message msg) {
  pthread_mutex_lock(&(q)->lock);
  int ret = mqueue__push_no_lock(q, msg);
  pthread_mutex_unlock(&(q)->lock);
  return ret;
}

int mqueue_push_many(message_queue *q, synth_message *msg, size_t count) {
  pthread_mutex_lock(&(q)->lock);
  int ret = 0;
  for (size_t i = 0; i < count; i++) {
	ret = mqueue__push_no_lock(q, msg[i]);
	if (ret != 0) {
	  break;
	}
  }
  pthread_mutex_unlock(&(q)->lock);
  return ret;
}
