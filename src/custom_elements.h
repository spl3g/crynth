#ifndef CUSTOM_ELEMENTS_H_
#define CUSTOM_ELEMENTS_H_

#include "clay.h"

typedef enum {
  CUSTOM_ELEMENT_TYPE_CIRCLE,
  CUSTOM_ELEMENT_TYPE_WAVE_SCREEN,
} CustomElementType;

typedef struct {
  float start_angle;
  float value;
} CircleData;

typedef struct {
  float *point_buffer;
  size_t buffer_len;
  size_t thickness;
} WaveScreenData;

typedef struct {
  CustomElementType type;

  union {
	CircleData circle;
	WaveScreenData wave_screen;
  };
} CustomElementData;

void handle_custom(Clay_BoundingBox bbox, Clay_CustomRenderData *config);

#endif // CUSTOM_ELEMENTS_H_
