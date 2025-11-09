#include "custom_elements.h"

#include "sokol/sokol_gfx.h"
#include "sokol/sokol_gl.h"
#include <stdio.h>
#include <math.h>

#define STEP_AMOUNT 128

typedef struct {
  float x;
  float y;
} Vec2;

static float _SIN_FULL[128] = {
  0.000000f, 0.049068f, 0.098017f, 0.146730f, 
  0.195090f, 0.242980f, 0.290285f, 0.336890f, 
  0.382683f, 0.427555f, 0.471397f, 0.514103f, 
  0.555570f, 0.595699f, 0.634393f, 0.671559f, 
  0.707107f, 0.740951f, 0.773010f, 0.803208f, 
  0.831470f, 0.857729f, 0.881921f, 0.903989f, 
  0.923880f, 0.941544f, 0.956940f, 0.970031f, 
  0.980785f, 0.989177f, 0.995185f, 0.998795f, 
  1.000000f, 0.998795f, 0.995185f, 0.989177f, 
  0.980785f, 0.970031f, 0.956940f, 0.941544f, 
  0.923880f, 0.903989f, 0.881921f, 0.857729f, 
  0.831470f, 0.803208f, 0.773010f, 0.740951f, 
  0.707107f, 0.671559f, 0.634393f, 0.595699f, 
  0.555570f, 0.514103f, 0.471397f, 0.427555f, 
  0.382683f, 0.336890f, 0.290285f, 0.242980f, 
  0.195090f, 0.146730f, 0.098017f, 0.049068f, 
  0.000000f, -0.049068f, -0.098017f, -0.146730f, 
  -0.195090f, -0.242980f, -0.290285f, -0.336890f, 
  -0.382683f, -0.427555f, -0.471397f, -0.514103f, 
  -0.555570f, -0.595699f, -0.634393f, -0.671559f, 
  -0.707107f, -0.740951f, -0.773010f, -0.803208f, 
  -0.831470f, -0.857729f, -0.881921f, -0.903989f, 
  -0.923880f, -0.941544f, -0.956940f, -0.970031f, 
  -0.980785f, -0.989177f, -0.995185f, -0.998795f, 
  -1.000000f, -0.998795f, -0.995185f, -0.989177f, 
  -0.980785f, -0.970031f, -0.956940f, -0.941544f, 
  -0.923880f, -0.903989f, -0.881921f, -0.857729f, 
  -0.831470f, -0.803208f, -0.773010f, -0.740951f, 
  -0.707107f, -0.671559f, -0.634393f, -0.595699f, 
  -0.555570f, -0.514103f, -0.471397f, -0.427555f, 
  -0.382683f, -0.336890f, -0.290285f, -0.242980f, 
  -0.195090f, -0.146730f, -0.098017f, -0.049068f, 
};

static float _COS_FULL[128] = {
  1.000000f, 0.998795f, 0.995185f, 0.989177f, 
  0.980785f, 0.970031f, 0.956940f, 0.941544f, 
  0.923880f, 0.903989f, 0.881921f, 0.857729f, 
  0.831470f, 0.803208f, 0.773010f, 0.740951f, 
  0.707107f, 0.671559f, 0.634393f, 0.595699f, 
  0.555570f, 0.514103f, 0.471397f, 0.427555f, 
  0.382683f, 0.336890f, 0.290285f, 0.242980f, 
  0.195090f, 0.146730f, 0.098017f, 0.049068f, 
  0.000000f, -0.049068f, -0.098017f, -0.146730f, 
  -0.195090f, -0.242980f, -0.290285f, -0.336890f, 
  -0.382683f, -0.427555f, -0.471397f, -0.514103f, 
  -0.555570f, -0.595699f, -0.634393f, -0.671559f, 
  -0.707107f, -0.740951f, -0.773010f, -0.803208f, 
  -0.831470f, -0.857729f, -0.881921f, -0.903989f, 
  -0.923880f, -0.941544f, -0.956940f, -0.970031f, 
  -0.980785f, -0.989177f, -0.995185f, -0.998795f, 
  -1.000000f, -0.998795f, -0.995185f, -0.989177f, 
  -0.980785f, -0.970031f, -0.956940f, -0.941544f, 
  -0.923880f, -0.903989f, -0.881921f, -0.857729f, 
  -0.831470f, -0.803208f, -0.773010f, -0.740951f, 
  -0.707107f, -0.671559f, -0.634393f, -0.595699f, 
  -0.555570f, -0.514103f, -0.471397f, -0.427555f, 
  -0.382683f, -0.336890f, -0.290285f, -0.242980f, 
  -0.195090f, -0.146730f, -0.098017f, -0.049068f, 
  -0.000000f, 0.049068f, 0.098017f, 0.146730f, 
  0.195090f, 0.242980f, 0.290285f, 0.336890f, 
  0.382683f, 0.427555f, 0.471397f, 0.514103f, 
  0.555570f, 0.595699f, 0.634393f, 0.671559f, 
  0.707107f, 0.740951f, 0.773010f, 0.803208f, 
  0.831470f, 0.857729f, 0.881921f, 0.903989f, 
  0.923880f, 0.941544f, 0.956940f, 0.970031f, 
  0.980785f, 0.989177f, 0.995185f, 0.998795f, 
};

static float STEP = (float)STEP_AMOUNT / 360.0f;


static void draw_thick_line(float x1, float y1, float x2, float y2, float thickness) {
  float rad = atan2f(y2 - y1, x2 - x1);

  float xoffset = -1 * thickness * sinf(rad);
  if (rad < 0) {
	xoffset *= -1;
  }
  float yoffset = thickness * cosf(rad);
  if (rad < 0) {
	yoffset *= -1;
  }

  /* float xoffset = sqrtf(powf(thickness, 2) * (1.0f - powf(sinf(M_PI / 2 - rad), 2))); */
  /* float yoffset = thickness * sinf(M_PI / 2 - rad); */
  /* /\* if (rad < 0) { *\/ */
  /* 	xoffset *= -1; */
  /* 	yoffset *= -1; */
  /* } */
  /* printf("rad: %f, x: %f, y: %f\n", rad, xoffset, yoffset); */

  float x1top = x1 - xoffset;
  float y1top = y1 - yoffset;

  float x2top = x2 - xoffset;
  float y2top = y2 - yoffset;

  float x1bottom = x1 + xoffset;
  float y1bottom = y1 + yoffset;

  float x2bottom = x2 + xoffset;
  float y2bottom = y2 + yoffset;
  /* printf("1t: (%f %f) 2t: (%f %f)\n" */
  /* 		 "1b: (%f %f) 2b: (%f %f)\n", */
  /* 		 x1top, y1top, */
  /* 		 x2top, y2top, */
  /* 		 x1bottom, y1bottom, */
  /* 		 x2bottom, y2bottom), */

  sgl_v2f(x1top, y1top);
  sgl_v2f(x1top, y1top);
  sgl_v2f(x2top, y2top);
  sgl_v2f(x1bottom, y1bottom);
  sgl_v2f(x2bottom, y2bottom);
  sgl_v2f(x2bottom, y2bottom);
}

static void draw_wave_screen(float x, float y, float h, float w, float *points, size_t len) {
  float samples_per_px = (float)len / w;

  float last_point = y + points[0] * h;
  for (int px = 1; px < (int)w; px++) {
	size_t start = (size_t)(px * samples_per_px);
	size_t end   = (size_t)((px + 1) * samples_per_px);
	if (end >= len) end = len - 1;

	float minv = 1.0f, maxv = -1.0f;
	for (size_t i = start; i <= end; i++) {
	  if (points[i] < minv) minv = points[i];
	  if (points[i] > maxv) maxv = points[i];
	}

	float y_min = y + h * (0.5f - 0.5f * maxv);
	float y_max = y + h * (0.5f - 0.5f * minv);

	if (y_min == y_max) {
	  printf("%d %f %d %f\n", x + px - 1, last_point, x + px, y_max);
	  draw_thick_line(x + px - 1, last_point, x + px, y_max, 5);  
	  last_point = y_max;
	}
  }
}

static void draw_circle(float x, float y, float h, float w, float start_angle, float end_angle) {
  float c_x = x + w / 2;
  float c_y = y + h / 2;
  float r = w > h ? h : w;
  r /= 2.0f;

  float rad_start = start_angle * (M_PI / 180.0f);
  float rad_end = end_angle * (M_PI / 180.0f);
  /* draw_thick_line(c_x, c_y, c_x+(r*cosf(rad_end)), c_y+(r*sinf(rad_end)), 5); */

  int segments = (int)(r * 1.5f);
  if (segments < 16) {
	segments = 16;
  }
  float angle_step = (rad_end - rad_start) / ((float)segments - 1);
  float angle;

  sgl_v2f(c_x, c_y);
  for (int i = 0; i < segments; i++) {
	angle = rad_start + angle_step * (float)i;
	sgl_v2f(c_x, c_y);
	sgl_v2f(c_x+(r*cosf(angle)), c_y+(r*sinf(angle)));
  }
  sgl_v2f(c_x+(r*cos(angle)), c_y+(r*sin(angle)));
}

static void _draw_circle(float x, float y, float h, float w, float start_angle, float end_angle) {
  float c_x = x + w / 2;
  float c_y = y + h / 2;
  float r = w > h ? h : w;
  r /= 2.0f;

  int start = start_angle * STEP;
  int end = end_angle * STEP;
  int amount;
  if (end_angle == start_angle) {
	amount = STEP_AMOUNT;
  } else {
	amount = (end - start + STEP_AMOUNT) % STEP_AMOUNT;
  }
  int idx = start;


  sgl_v2f(c_x, c_y);
  for (int i = 0; i < amount; i++) {
	sgl_v2f(c_x, c_y);
	sgl_v2f(c_x+(r*_COS_FULL[idx]), c_y+(r*_SIN_FULL[idx]));
	idx++;
	idx %= STEP_AMOUNT;
  }
  sgl_v2f(c_x, c_y);
  sgl_v2f(c_x+(r*_COS_FULL[idx]), c_y+(r*_SIN_FULL[idx]));
  sgl_v2f(c_x+(r*_COS_FULL[idx]), c_y+(r*_SIN_FULL[idx]));
}

void handle_custom(Clay_BoundingBox bbox, Clay_CustomRenderData *config) {
  CustomElementData *custom_data = config->customData;
  
  switch (custom_data->type) {
  case CUSTOM_ELEMENT_TYPE_CIRCLE: {
	CircleData circle_data = custom_data->circle;

	float start_angle = circle_data.start_angle;
	/* float end_angle = fmod(circle_data.value * 360 + start_angle + 360, 360); */
	float end_angle = circle_data.value * 360 + start_angle;
	end_angle = end_angle > start_angle ? end_angle : end_angle + 360;
	
	sgl_c4f(config->backgroundColor.r / 255.0f,
			config->backgroundColor.g / 255.0f,
			config->backgroundColor.b / 255.0f,
			config->backgroundColor.a / 255.0f);

	sgl_begin_triangle_strip();
	draw_circle(bbox.x, bbox.y, bbox.height, bbox.width,
				start_angle, end_angle);
	sgl_end();
	break;
  }
  case CUSTOM_ELEMENT_TYPE_WAVE_SCREEN: {
	WaveScreenData wave_data = custom_data->wave_screen;
	sgl_c4f(config->backgroundColor.r / 255.0f,
			config->backgroundColor.g / 255.0f,
			config->backgroundColor.b / 255.0f,
			config->backgroundColor.a / 255.0f);
	sgl_begin_triangle_strip();
	/* draw_thick_line(bbox.x, bbox.y, */
	/* 				bbox.x+bbox.width, bbox.y+bbox.height, */
	/* 				20); */
	draw_wave_screen(bbox.x, bbox.y,
					 bbox.height, bbox.width,
					 wave_data.point_buffer, wave_data.buffer_len);
	sgl_end();

  }
  }  
}
