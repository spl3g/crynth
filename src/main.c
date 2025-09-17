#include <stdio.h>
#include <alsa/asoundlib.h>
#include <math.h>
#include <raylib.h>
#include <pthread.h>
#include <stdbool.h>

#define SAMPLE_RATE 48000
#define PERIOD_SIZE 480
#define AMPLITUDE 10000

#define check(ret)                                                          \
    do {                                                                    \
        int res = (ret);                                                    \
        if (res < 0) {                                                      \
            fprintf(stderr, "%s:%d ERROR: %s (%d)\n",                       \
                __FILE__, __LINE__, snd_strerror(res), res);                \
            exit(1);                                                        \
        }                                                                   \
    } while (0)

typedef struct {
  snd_pcm_t * pcm;
  float *freqs;
  size_t freqs_count;
  bool should_stop;
} sound_thread_meta;

int set_hw_params(snd_pcm_t *pcm) {
  snd_pcm_hw_params_t *hw_params;

  snd_pcm_hw_params_alloca(&hw_params);

  check(snd_pcm_hw_params_any(pcm, hw_params));

  unsigned int resample = 1;
  check(snd_pcm_hw_params_set_rate_resample(pcm, hw_params, resample));
  check(snd_pcm_hw_params_set_access(pcm, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED));
  check(snd_pcm_hw_params_set_format(pcm, hw_params, SND_PCM_FORMAT_S16_LE));
  check(snd_pcm_hw_params_set_channels(pcm, hw_params, 1));
  check(snd_pcm_hw_params_set_rate(pcm, hw_params, SAMPLE_RATE, 0));
  snd_pcm_uframes_t period_size = PERIOD_SIZE;
  check(snd_pcm_hw_params_set_period_size_near(pcm, hw_params, &period_size, 0));
  snd_pcm_uframes_t buffer_size = period_size * 3;
  check(snd_pcm_hw_params_set_buffer_size_near(pcm, hw_params, &buffer_size));
  check(snd_pcm_hw_params(pcm, hw_params));
  
  return 0;
}

short *gen_sine(short *buffer, size_t sample_count, float freq, float *phase) {
  for (size_t i = 0; i < sample_count; i++) {
	buffer[i] = AMPLITUDE * sinf(*phase);
	*phase += 2 * M_PI * freq / SAMPLE_RATE;
	if (*phase >= 2 * M_PI) *phase -= 2 * M_PI;
  }

  return buffer;
}

short *gen_square(short *buffer, size_t sample_count, float freq, float *phase) {
	int samples_full_cycle = (float)SAMPLE_RATE / freq;
	int samples_half_cycle = samples_full_cycle / 2.0f;
	for (size_t i = 0; i < sample_count; i++) {
		buffer[i] = *phase < samples_half_cycle ? AMPLITUDE : -AMPLITUDE;
		*phase = (((int)*phase + 1) % samples_full_cycle);
	}
	return buffer;
}

short *gen_saw(short *buffer, size_t sample_count, float freq, float *phase) {
  int samples_full_cycle = (float)SAMPLE_RATE / freq;
  int step = AMPLITUDE / samples_full_cycle;

  for (size_t i = 0; i < sample_count; i++) {
	buffer[i] = *phase;

	*phase += step;
	if (*phase >= AMPLITUDE) {
	  *phase = -AMPLITUDE;
	}
  }

  return buffer;
}

void *sound_thread_generate(void *ptr) {
  sound_thread_meta *meta = ptr;
  
  short buffer[PERIOD_SIZE];
  float phase = 0.0f;

  float prev_freq = meta->freqs[0];

  while (!meta->should_stop) {
	if (meta->freqs_count <= 0) {
	  continue;
	}
	
	float freq = meta->freqs[0];
	if (freq != prev_freq) {
	  printf("%f\n", freq);
	  prev_freq = freq;
	}
	
	gen_saw(buffer, PERIOD_SIZE, freq, &phase);

	snd_pcm_sframes_t written = snd_pcm_writei(meta->pcm, buffer, PERIOD_SIZE);
	if (written < 0) {
	  snd_pcm_prepare(meta->pcm); // recover from xrun
	}
  }

  return NULL;
}

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

  float freqs[10];

  freqs[0] = 220;

  sound_thread_meta soundgen = {
	.pcm = sound_device,
	.freqs = freqs,
	.freqs_count = 1,
	.should_stop = false,
  };

  pthread_t sound_thread;
  pthread_create(&sound_thread, NULL, sound_thread_generate, &soundgen);

  while (!WindowShouldClose()) {
	if (IsKeyDown(KEY_SPACE)) {
	  soundgen.freqs[0] = 900;
	} else {
	  soundgen.freqs[0] = 500;
	}

	BeginDrawing();
	ClearBackground(RAYWHITE);
	EndDrawing();
  }

  soundgen.should_stop = true;
  CloseWindow();

  pthread_join(sound_thread, NULL);

  check(snd_pcm_drop(sound_device));
  check(snd_pcm_close(sound_device));

  return 0;
}
