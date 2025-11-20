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

typedef struct {
  Vec2 point;
  float angle;
} WavePoint;


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
  float rad = atan2f(y2 - y1, x2 - x1) + M_PI;

  float xoffset = -1 * thickness * sinf(rad);
  float yoffset = thickness * cosf(rad);

  /* float xoffset = sqrtf(powf(thickness, 2) * (1.0f - powf(sinf(M_PI / 2 - rad), 2))); */
  /* float yoffset = thickness * sinf(M_PI / 2 - rad); */
  /* /\* if (rad < 0) { *\/ */
  /* 	xoffset *= -1; */
  /* 	yoffset *= -1; */
  /* } */

  float x1top = x1 - xoffset;
  float y1top = y1 - yoffset;

  float x2top = x2 - xoffset;
  float y2top = y2 - yoffset;

  float x1bottom = x1 + xoffset;
  float y1bottom = y1 + yoffset;

  float x2bottom = x2 + xoffset;
  float y2bottom = y2 + yoffset;

  sgl_v2f(x1top, y1top);
  sgl_v2f(x1top, y1top);
  sgl_v2f(x2top, y2top);
  sgl_v2f(x1bottom, y1bottom);
  sgl_v2f(x2bottom, y2bottom);
  sgl_v2f(x2bottom, y2bottom);
}

/* static void _draw_thick_points(WavePoint points[3], int thickness) { */
/* } */

static void draw_wave_screen(float x, float y, float h, float w, int thickness, float *points, size_t len) {
  float samples_per_px = (float)len / w;

  WavePoint wave_points[(int)w*4];

  wave_points[0] = (WavePoint){
	.point = (Vec2){
	  .x = x,
	  .y = y + h * (0.5f - 0.5f * points[0]),
	},
	.angle = 0,
  };

  int point_len = 1;
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
	
	if (y_max - y_min < 5) {
	  float point = (y_min + y_max) / 2;
	  wave_points[point_len++] = (WavePoint){
		.point = (Vec2){x+px, point},
	  };
	} else {
	  float line[4];
	  float ydel = (y_max - y_min);

	  if (points[px+1] - minv < points[px+1] - maxv) {
		line[0] = y_min;
		line[1] = ydel / 15 + y_min;
		line[2] = ydel * 14 / 15 + y_min;
		line[3] = y_max;
	  } else {
		line[0] = y_max;
		line[1] = ydel * 14 / 15 + y_min;
		line[2] = ydel / 15 + y_min;
		line[3] = y_min;
	  }

	  for (int i = 0; i < 4; i++) {
		wave_points[point_len++] = (WavePoint){
		  .point = (Vec2){x+px, line[i]},
		};
	  }
	}
  }

  /* float step = len / w; */

  for (int i = 1; i < point_len; i++) {
	/* int i = px * step; */

	/* Vec2 p = { */
	/*   .x = x + i, */
	/*   .y = y + h * (0.5f - 0.5f * points[i]), */
	/* }; */
	Vec2 p = wave_points[i].point;
	Vec2 pp = wave_points[i-1].point;

	Vec2 pn = wave_points[i+1].point;

	/* Vec2 pn = { */
	/*   .x = x + i + 1, */
	/*   .y = y + h * (0.5f - 0.5f * points[(int)((i + 1) * step)]), */
	/* }; */

	float next_angle = atan2f(pn.y - p.y, pn.x - p.x) + M_PI;
	float prev_angle = atan2f(p.y - pp.y, p.x - pp.x) + M_PI;
	float angle = (prev_angle + next_angle) / 2;

	Vec2 poffset = {
	  .x = -1 * thickness * sinf(wave_points[i-1].angle),
	  .y = thickness * cosf(wave_points[i-1].angle),
	};

	
	Vec2 coffset = {
	  .x = -1 * thickness * sinf(angle),
	  .y = thickness * cosf(angle),
	};

	if (i == 1) {
	  sgl_v2f(pp.x - poffset.x, pp.y - poffset.y);
	}
	sgl_v2f(pp.x - poffset.x, pp.y - poffset.y);
	sgl_v2f(p.x - coffset.x, p.y - coffset.y);
	sgl_v2f(pp.x + poffset.x, pp.y + poffset.y);
	sgl_v2f(p.x + coffset.x, p.y + coffset.y);

	if (i == (int)w - 1) {
	  sgl_v2f(p.x + coffset.x, p.y + coffset.y);
	}

	wave_points[i] = (WavePoint){
	  .point = p,
	  .angle = angle,
	};
  }
}

static void draw_circle(float x, float y, float h, float w, float start_angle, float end_angle) {
  float c_x = x + w / 2;
  float c_y = y + h / 2;
  float r = w > h ? h : w;
  r /= 2.0f;

  float rad_start = start_angle * (M_PI / 180.0f);
  float rad_end = end_angle * (M_PI / 180.0f);

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
	draw_wave_screen(bbox.x, bbox.y,
					 bbox.height, bbox.width, wave_data.thickness,
					 wave_data.point_buffer, wave_data.buffer_len);
	sgl_end();

  }
  }  
}
