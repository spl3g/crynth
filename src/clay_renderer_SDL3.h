#ifndef RENDERER_H_
#define RENDERER_H_

#include "clay.h"
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <SDL3_image/SDL_image.h>

typedef struct {
  SDL_Renderer *renderer;
  TTF_TextEngine *textEngine;
  TTF_Font **fonts;
} Clay_SDL3RendererData;

typedef enum {
  CUSTOM_ELEMENT_TYPE_CIRCLE,
} CustomElementType;

typedef struct {
  float start_angle;
  float value;
  Clay_Color color;
} CircleData;

typedef struct {
  CustomElementType type;

  union {
	CircleData circle;
  };
} CustomElementData;

void SDL_Clay_RenderClayCommands(Clay_SDL3RendererData *rendererData, Clay_RenderCommandArray *rcommands);

#endif // RENDERER_H_
