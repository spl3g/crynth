#define MIDI_FREQS_LIST
#include "sounds.h"

float envelope_next(envelope *env) {
  float value;
  bool next_state = false;

  env->counter++;

  switch (env->state) {
  case ENV_OFF: {
	return 0;
  }
  case ENV_ATTACK: {
	if (env->counter >= env->params.attack_time) {
	  next_state = true;
	}

	if (env->current_inc == 0) {
	  env->current_inc = (1.0 - env->release_value) / (float)env->params.attack_time;
	}
	
	value = env->release_value + env->current_inc * (float)env->counter;
	break;
  }
  case ENV_DECAY: {
	if (env->counter >= env->params.decay_time) {
	  next_state = true;
	}

	if (env->current_inc == 0) {
	  env->current_inc = (1.0 - env->params.sustain_level) / (float)env->params.decay_time;
	}
	
	value = 1.0 - env->current_inc * (float)env->counter;
	break;
  }
  case ENV_SUSTAIN: {
	value = env->params.sustain_level;
	break;
  }
  case ENV_RELEASE: {
	if (env->counter >= env->params.release_time) {
	  env->state = ENV_OFF;
	  env->counter = env->current_inc = 0;
	  return 0;
	}

	if (env->current_inc == 0) {
	  env->current_inc = env->params.sustain_level / (float)env->params.release_time;
	}
	
	value = env->params.sustain_level - env->current_inc * (float)env->counter;
	env->release_value = value;
	break;
  }
  }

  if (next_state) {
	env->counter = env->current_inc = 0;
	env->state++;
  }
  return value;
}

void envelope_note_on(envelope_params params, envelope *env) {
  env->state = ENV_ATTACK;
  env->counter = 0;
  env->params = params;
}

void envelope_note_off(envelope *env) {
  env->state = ENV_RELEASE;
  env->counter = 0;
}

float osc_sine_get(float phase) {
  return sinf(phase);
}

float osc_square_get(float phase) {
    return sinf(phase) > 0 ? 1 : -1;
}

float osc_saw_get(float phase) {
  return (phase / M_PI) - 1;
}

oscilator_func osc_get(oscilator_type type) {
  switch (type) {
  case OSC_SINE:
    return osc_sine_get;
  case OSC_SAW:
    return osc_saw_get;
  case OSC_SQUARE:
    return osc_square_get;
  default:
    return osc_sine_get;
  }
}

void set_note_on(synth_params *params, synth_voices *voices, size_t note_id) {
  if (note_id >= voices->size) {
    return;
  }
  envelope_note_on(params->envelope_params, &voices->buffer[note_id].envelope);
  voices->buffer[note_id].active = true;
  params->last_freq = voices->buffer[note_id].freq;
}

void set_note_off(synth_voices *voices, size_t note_id) {
  if (note_id >= voices->size) {
    return;
  }
  envelope_note_off(&voices->buffer[note_id].envelope);
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
  case PARAM_ATTACK: {
	params->envelope_params.attack_time = (int)(value / 1000 * SAMPLE_RATE);
	break;
  }
  case PARAM_DECAY: {
	params->envelope_params.decay_time = (int)(value / 1000 * SAMPLE_RATE);
	break;
  }
  case PARAM_SUSTAIN: {
	params->envelope_params.sustain_level = value;
	break;
  }
  case PARAM_RELEASE: {
	params->envelope_params.release_time = (int)(value / 1000 * SAMPLE_RATE);
	break;
  }
  }
}

void generate_voices(synth_voices *voices, synth_params *params, float *buffer, size_t buffer_size) {
  oscilator_func oscilator = osc_get(params->oscilator_type);
  for (size_t i = 0; i < voices->size; i++) {
    synth_voice *voice = &voices->buffer[i];
    if (!voice->active) {
      continue;
    }

	if (voice->phase_inc == 0) {
	  voice->phase_inc = 2 * M_PI * voice->freq / SAMPLE_RATE;
	}

	for (size_t j = 0; j < buffer_size; j++) {
	  float env_value = envelope_next(&voice->envelope);
	  if (env_value == 0) {
		voice->active = false;
		break;
	  }
	  
	  buffer[j] += oscilator(voice->phase) * env_value;

	  voice->phase += voice->phase_inc;
	  if (voice->phase >= 2 * M_PI)
		voice->phase -= 2 * M_PI;
	}
  }
}

void print_wave(oscilator_func osc, float freq, size_t size) {
  float phase = 0;
  float phase_inc = 2 * M_PI * freq / SAMPLE_RATE;
  
  printf("float wave_buffer[%d] = {\n", size);
  for (size_t j = 0; j < size; j++) {
	printf("  %f,\n", osc(phase));

	phase += phase_inc;
	if (phase >= 2 * M_PI)
	  phase -= 2 * M_PI;
	
  }
  printf("}\n");
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
    output_buffer[i] = scratch_buffer[i] * 32767;
  }
}

void sound_loop_start(snd_pcm_t *pcm, message_queue *queue, WaveData *wave_data, synth_voices *voices) {
  synth_params params;
  short output_buffer[PERIOD_SIZE];
  float scratch_buffer[PERIOD_SIZE];
  float display_buffer[DISPLAY_SAMPLES];
  size_t display_write_pos = 0;

  while (true) {
    synth_message msg;
    while (mqueue_get(queue, &msg) == 0) {
      switch (msg.type) {
      case MSG_NOTE_ON: {
        size_t note_id = msg.note.note_id;
        set_note_on(&params, voices, note_id);
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
        set_param(&params, type, value);
        break;
      }
      case MSG_STOP: {
        goto stop;
      }
      }
    }

    memset(&output_buffer, 0, PERIOD_SIZE * sizeof(short));
    memset(&scratch_buffer, 0, PERIOD_SIZE * sizeof(float));

    generate_voices(voices, &params, scratch_buffer, PERIOD_SIZE);

    post_process(&params, scratch_buffer, PERIOD_SIZE);
    prepare_output(scratch_buffer, output_buffer, PERIOD_SIZE);

	if (display_write_pos + PERIOD_SIZE > DISPLAY_SAMPLES) {
	  size_t diff = DISPLAY_SAMPLES - display_write_pos;
	  memcpy(&display_buffer[display_write_pos], scratch_buffer, diff * sizeof(float));
	  memcpy(display_buffer, &scratch_buffer[diff], (PERIOD_SIZE - diff) * sizeof(float));
	} else {
	  memcpy(&display_buffer[display_write_pos], scratch_buffer, PERIOD_SIZE * sizeof(float));
	}
	
	display_write_pos = (display_write_pos + PERIOD_SIZE) % DISPLAY_SAMPLES;

	memcpy(wave_data->buffers[wave_data->write_index], display_buffer, DISPLAY_SAMPLES * sizeof(float));
	atomic_store(&wave_data->write_index, 1 - atomic_load(&wave_data->write_index));
	wave_data->freq = params.last_freq;

	int period_size = PERIOD_SIZE;
	short *ptr = output_buffer;
	
	while (period_size > 0) {
      snd_pcm_sframes_t written = snd_pcm_writei(pcm, ptr, period_size);
      if (written < 0) {
		printf("xrun\n");
		snd_pcm_prepare(pcm); // recover from xrun
		break;
      }
	  ptr += written;
	  period_size -= written;
	}
  }
stop:
  return;
}

void fill_voices(synth_voice *voices, MidiNote *freqs, size_t freqs_amount) {
  for (size_t i = 0; i < freqs_amount; i++) {
    voices[i] = (synth_voice){
        .active = false,
        .freq = freqs[i].freq,
        .phase = 0,
        .phase_inc = 0,
		.envelope = {0},
    };
  }
}

void *sound_thread_start(void *ptr) {
  sound_thread_meta *meta = ptr;

  synth_voice buffer[12];
  fill_voices(buffer, &midi_freqs[49], 12);

  synth_voices voices = {
      .buffer = buffer,
      .size = 12,
  };

  sound_loop_start(meta->pcm, meta->queue, meta->wave_data, &voices);

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
