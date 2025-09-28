#include "sounds.h"

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

/* osc_triangle_gen */

void osc_sine_gen(float *buffer, size_t sample_count, float *phase,
                  float phase_inc) {
  for (size_t i = 0; i < sample_count; i++) {
    buffer[i] += sinf(*phase);
    *phase += phase_inc;
    if (*phase >= 2 * M_PI)
      *phase -= 2 * M_PI;
  }
}

void osc_square_gen(float *buffer, size_t sample_count, float *phase,
                    float phase_inc) {
  for (size_t i = 0; i < sample_count; i++) {
    buffer[i] += sinf(*phase) ? 1 : -1;
    *phase += phase_inc;
    if (*phase >= 2 * M_PI)
      *phase -= 2 * M_PI;
  }
}

void osc_saw_gen(float *buffer, size_t sample_count, float *phase,
                 float phase_inc) {
  for (size_t i = 0; i < sample_count; i++) {
    buffer[i] += (*phase / M_PI) - 1;
    *phase += phase_inc;
    if (*phase >= 2 * M_PI)
      *phase -= 2 * M_PI;
  }
}

oscilator_func osc_get(oscilator_type type) {
  switch (type) {
  case OSC_SINE:
    return osc_sine_gen;
  case OSC_SAW:
    return osc_saw_gen;
  case OSC_SQUARE:
    return osc_square_gen;
  default:
    return osc_sine_gen;
  }
}

void set_note_on(synth_voices *voices, size_t note_id) {
  if (note_id >= voices->size) {
    return;
  }
  voices->buffer[note_id].active = true;
}

void set_note_off(synth_voices *voices, size_t note_id) {
  if (note_id >= voices->size) {
    return;
  }
  voices->buffer[note_id].active = false;
}

void set_all_notes_off(synth_voices *voices) {
  for (size_t i = 0; i < voices->size; i++) {
    if (voices->buffer[i].active) {
      voices->buffer[i].active = false;
    }
  }
}

void set_param(synth_params *params, param_type type, float value) {
  switch (type) {
  case PARAM_OSC: {
    params->oscilator_type = (int)value;
    break;
  }
  case PARAM_VOLUME: {
    params->master_volume = value;
    break;
  }
  }
}

void generate_voices(synth_voices *voices, synth_params *params, float *buffer,
                     size_t buffer_size) {
  oscilator_func oscilator = osc_get(params->oscilator_type);
  for (size_t i = 0; i < voices->size; i++) {
    synth_voice *voice = &voices->buffer[i];
    if (!voice->active) {
      continue;
    }

    if (voice->phase_inc == 0) {
      voice->phase_inc = 2 * M_PI * voice->freq / SAMPLE_RATE;
    }
    oscilator(buffer, buffer_size, &voice->phase, voice->phase_inc);
  }
}

void post_process(synth_params *params, float *scratch_buffer,
                  size_t buffer_size) {
  for (size_t i = 0; i < buffer_size; i++) {
    scratch_buffer[i] *= params->master_volume;
  }
}

void prepare_output(float *scratch_buffer, short *output_buffer,
                    size_t buffer_size) {
  for (size_t i = 0; i < buffer_size; i++) {
    output_buffer[i] = scratch_buffer[i] * 0.2f * 32767;
  }
}

void sound_loop_start(snd_pcm_t *pcm, message_queue *queue,
                      synth_voices *voices, synth_params *params) {
  bool should_stop = false;

  short output_buffer[PERIOD_SIZE];
  float scratch_buffer[PERIOD_SIZE];

  while (!should_stop) {
    synth_message msg;
    while (mqueue_get(queue, &msg) == 0 && !should_stop) {
      switch (msg.type) {
      case MSG_NOTE_ON: {
        size_t note_id = msg.note.note_id;
        set_note_on(voices, note_id);
        break;
      }
      case MSG_NOTE_OFF: {
        size_t note_id = msg.note.note_id;
        set_note_off(voices, note_id);
        break;
      }
      case MSG_ALL_NOTES_OFF: {
        set_all_notes_off(voices);
        break;
      }
      case MSG_PARAM_CHANGE: {
        param_type type = msg.param_change.param_type;
        float value = msg.param_change.value;
        set_param(params, type, value);
        break;
      }
      case MSG_STOP: {
        should_stop = true;
      }
      }
    }

    memset(&output_buffer, 0, PERIOD_SIZE * sizeof(short));
    memset(&scratch_buffer, 0, PERIOD_SIZE * sizeof(float));

    generate_voices(voices, params, scratch_buffer, PERIOD_SIZE);

    /* post_process(params, scratch_buffer, PERIOD_SIZE); */
    prepare_output(scratch_buffer, output_buffer, PERIOD_SIZE);

    snd_pcm_sframes_t written = snd_pcm_writei(pcm, output_buffer, PERIOD_SIZE);
    if (written < 0) {
      printf("xrun\n");
      snd_pcm_prepare(pcm); // recover from xrun
    }
  }
}

void fill_voices(synth_voice *voices, float *freqs, size_t freqs_amount) {
  for (size_t i = 0; i < freqs_amount; i++) {
    voices[i] = (synth_voice){
        .active = false,
        .freq = freqs[i],
        .phase = 0,
        .phase_inc = 0,
    };
  }
}

void *sound_thread_start(void *ptr) {
  sound_thread_meta *meta = ptr;

  float freqs[12] = {
      261.63f, // c
      277.18f, // c#
      293.66f, // e
      311.13f, // e#
      329.63f, // d
      349.23f, // f
      369.99f, // f#
      392,     // g
      415.3f,  // g#
      440,     // a
      466.16,  // a#
      493.88,  // b
  };

  synth_voice buffer[12];
  fill_voices(buffer, freqs, 12);

  synth_voices voices = {
      .buffer = buffer,
      .size = 12,
  };
  synth_params params = {
      .oscilator_type = OSC_SINE,
  };

  sound_loop_start(meta->pcm, meta->queue, &voices, &params);

  check(snd_pcm_drop(meta->pcm));
  return NULL;
}

int set_hw_params(snd_pcm_t *pcm) {
  snd_pcm_hw_params_t *hw_params;

  snd_pcm_hw_params_alloca(&hw_params);

  check(snd_pcm_hw_params_any(pcm, hw_params));

  unsigned int resample = 1;
  check(snd_pcm_hw_params_set_rate_resample(pcm, hw_params, resample));
  check(snd_pcm_hw_params_set_access(pcm, hw_params,
                                     SND_PCM_ACCESS_RW_INTERLEAVED));
  check(snd_pcm_hw_params_set_format(pcm, hw_params, SND_PCM_FORMAT_S16_LE));
  check(snd_pcm_hw_params_set_channels(pcm, hw_params, 1));
  check(snd_pcm_hw_params_set_rate(pcm, hw_params, SAMPLE_RATE, 0));
  snd_pcm_uframes_t period_size = PERIOD_SIZE;
  check(
      snd_pcm_hw_params_set_period_size_near(pcm, hw_params, &period_size, 0));
  snd_pcm_uframes_t buffer_size = period_size * 4;
  check(snd_pcm_hw_params_set_buffer_size_near(pcm, hw_params, &buffer_size));
  check(snd_pcm_hw_params(pcm, hw_params));

  return 0;
}
