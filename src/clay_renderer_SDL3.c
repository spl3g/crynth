#include "clay_renderer_SDL3.h"
#include <stdio.h>

/* Global for convenience. Even in 4K this is enough for smooth curves (low radius or rect size coupled with
 * no AA or low resolution might make it appear as jagged curves) */
static int NUM_CIRCLE_SEGMENTS = 16;

//all rendering is performed by a single SDL call, avoiding multiple RenderRect + plumbing choice for circles.
static void SDL_Clay_RenderFillRoundedRect(Clay_SDL3RendererData *rendererData, const SDL_FRect rect, const float cornerRadius, const Clay_Color _color) {
  const SDL_FColor color = { _color.r/255, _color.g/255, _color.b/255, _color.a/255 };

  int indexCount = 0, vertexCount = 0;

  const float minRadius = SDL_min(rect.w, rect.h) / 2.0f;
  const float clampedRadius = SDL_min(cornerRadius, minRadius);

  const int numCircleSegments = SDL_max(NUM_CIRCLE_SEGMENTS, (int) clampedRadius * 0.5f);

  int totalVertices = 4 + (4 * (numCircleSegments * 2)) + 2*4;
  int totalIndices = 6 + (4 * (numCircleSegments * 3)) + 6*4;

  SDL_Vertex vertices[totalVertices];
  int indices[totalIndices];

  //define center rectangle
  vertices[vertexCount++] = (SDL_Vertex){ {rect.x + clampedRadius, rect.y + clampedRadius}, color, {0, 0} }; //0 center TL
  vertices[vertexCount++] = (SDL_Vertex){ {rect.x + rect.w - clampedRadius, rect.y + clampedRadius}, color, {1, 0} }; //1 center TR
  vertices[vertexCount++] = (SDL_Vertex){ {rect.x + rect.w - clampedRadius, rect.y + rect.h - clampedRadius}, color, {1, 1} }; //2 center BR
  vertices[vertexCount++] = (SDL_Vertex){ {rect.x + clampedRadius, rect.y + rect.h - clampedRadius}, color, {0, 1} }; //3 center BL

  indices[indexCount++] = 0;
  indices[indexCount++] = 1;
  indices[indexCount++] = 3;
  indices[indexCount++] = 1;
  indices[indexCount++] = 2;
  indices[indexCount++] = 3;

  //define rounded corners as triangle fans
  const float step = (SDL_PI_F/2) / numCircleSegments;
  for (int i = 0; i < numCircleSegments; i++) {
    const float angle1 = (float)i * step;
    const float angle2 = ((float)i + 1.0f) * step;

    for (int j = 0; j < 4; j++) {  // Iterate over four corners
      float cx, cy, signX, signY;

      switch (j) {
      case 0: cx = rect.x + clampedRadius; cy = rect.y + clampedRadius; signX = -1; signY = -1; break; // Top-left
      case 1: cx = rect.x + rect.w - clampedRadius; cy = rect.y + clampedRadius; signX = 1; signY = -1; break; // Top-right
      case 2: cx = rect.x + rect.w - clampedRadius; cy = rect.y + rect.h - clampedRadius; signX = 1; signY = 1; break; // Bottom-right
      case 3: cx = rect.x + clampedRadius; cy = rect.y + rect.h - clampedRadius; signX = -1; signY = 1; break; // Bottom-left
      default: return;
      }

      vertices[vertexCount++] = (SDL_Vertex){ {cx + SDL_cosf(angle1) * clampedRadius * signX, cy + SDL_sinf(angle1) * clampedRadius * signY}, color, {0, 0} };
      vertices[vertexCount++] = (SDL_Vertex){ {cx + SDL_cosf(angle2) * clampedRadius * signX, cy + SDL_sinf(angle2) * clampedRadius * signY}, color, {0, 0} };

      indices[indexCount++] = j;  // Connect to corresponding central rectangle vertex
      indices[indexCount++] = vertexCount - 2;
      indices[indexCount++] = vertexCount - 1;
    }
  }

  //Define edge rectangles
  // Top edge
  vertices[vertexCount++] = (SDL_Vertex){ {rect.x + clampedRadius, rect.y}, color, {0, 0} }; //TL
  vertices[vertexCount++] = (SDL_Vertex){ {rect.x + rect.w - clampedRadius, rect.y}, color, {1, 0} }; //TR

  indices[indexCount++] = 0;
  indices[indexCount++] = vertexCount - 2; //TL
  indices[indexCount++] = vertexCount - 1; //TR
  indices[indexCount++] = 1;
  indices[indexCount++] = 0;
  indices[indexCount++] = vertexCount - 1; //TR
  // Right edge
  vertices[vertexCount++] = (SDL_Vertex){ {rect.x + rect.w, rect.y + clampedRadius}, color, {1, 0} }; //RT
  vertices[vertexCount++] = (SDL_Vertex){ {rect.x + rect.w, rect.y + rect.h - clampedRadius}, color, {1, 1} }; //RB

  indices[indexCount++] = 1;
  indices[indexCount++] = vertexCount - 2; //RT
  indices[indexCount++] = vertexCount - 1; //RB
  indices[indexCount++] = 2;
  indices[indexCount++] = 1;
  indices[indexCount++] = vertexCount - 1; //RB
  // Bottom edge
  vertices[vertexCount++] = (SDL_Vertex){ {rect.x + rect.w - clampedRadius, rect.y + rect.h}, color, {1, 1} }; //BR
  vertices[vertexCount++] = (SDL_Vertex){ {rect.x + clampedRadius, rect.y + rect.h}, color, {0, 1} }; //BL

  indices[indexCount++] = 2;
  indices[indexCount++] = vertexCount - 2; //BR
  indices[indexCount++] = vertexCount - 1; //BL
  indices[indexCount++] = 3;
  indices[indexCount++] = 2;
  indices[indexCount++] = vertexCount - 1; //BL
  // Left edge
  vertices[vertexCount++] = (SDL_Vertex){ {rect.x, rect.y + rect.h - clampedRadius}, color, {0, 1} }; //LB
  vertices[vertexCount++] = (SDL_Vertex){ {rect.x, rect.y + clampedRadius}, color, {0, 0} }; //LT

  indices[indexCount++] = 3;
  indices[indexCount++] = vertexCount - 2; //LB
  indices[indexCount++] = vertexCount - 1; //LT
  indices[indexCount++] = 0;
  indices[indexCount++] = 3;
  indices[indexCount++] = vertexCount - 1; //LT

  // Render everything
  SDL_RenderGeometry(rendererData->renderer, NULL, vertices, vertexCount, indices, indexCount);
}

static void SDL_Clay_RenderArc(Clay_SDL3RendererData *rendererData, const SDL_FPoint center, const float radius, const float startAngle, const float endAngle, const float thickness, const Clay_Color color) {
  SDL_SetRenderDrawColor(rendererData->renderer, color.r, color.g, color.b, color.a);

  const float radStart = startAngle * (SDL_PI_F / 180.0f);
  const float radEnd = endAngle * (SDL_PI_F / 180.0f);

  const int numCircleSegments = SDL_max(NUM_CIRCLE_SEGMENTS, (int)(radius * 1.5f)); //increase circle segments for larger circles, 1.5 is arbitrary.

  const float angleStep = (radEnd - radStart) / (float)numCircleSegments;
  const float thicknessStep = 0.4f; //arbitrary value to avoid overlapping lines. Changing THICKNESS_STEP or numCircleSegments might cause artifacts.

  for (float t = thicknessStep; t < thickness - thicknessStep; t += thicknessStep) {
    SDL_FPoint points[numCircleSegments + 1];
    const float clampedRadius = SDL_max(radius - t, 1.0f);

    for (int i = 0; i <= numCircleSegments; i++) {
      const float angle = radStart + i * angleStep;
      points[i] = (SDL_FPoint){
        SDL_roundf(center.x + SDL_cosf(angle) * clampedRadius),
        SDL_roundf(center.y + SDL_sinf(angle) * clampedRadius) };
    }
    SDL_RenderLines(rendererData->renderer, points, numCircleSegments + 1);
  }
}

void SDL_Clay_RenderCircle(SDL_Renderer *renderer, float x, float y, float width, float height, float start_angle, float end_angle, const Clay_Color _color) {
  const SDL_FColor color = { _color.r/255, _color.g/255, _color.b/255, _color.a/255 };
  float center_x = x + width / 2;
  float center_y = y + width / 2;
  float min_diameter = width > height ? height : width;
  float radius = min_diameter / 2;

  float rad_start = start_angle * (SDL_PI_F / 180.0f);
  float rad_end = end_angle * (SDL_PI_F / 180.0f);

  const int segments = SDL_max(16, (int)(radius * 1.5f));
  SDL_Vertex vertices[segments + 1];

  int vertexCount = 0, indexCount = 0;

  vertices[vertexCount++] = (SDL_Vertex){
	.position = { center_x, center_y },
	.color = color
  };

  float angle_step = (rad_end - rad_start) / ((float)segments - 1);

  for (int i = 0; i < segments; i++) {
	float angle = rad_start + (float)i * angle_step;
	vertices[vertexCount++] = (SDL_Vertex){
	  .position = {
		center_x + radius * SDL_cosf(angle),
		center_y + radius * SDL_sinf(angle)
	  },
	  .color = color
	};
  }

  int indices[segments * 3];
  for (int j = 0; j < segments - 1; j++) {
	indices[indexCount++] = 0;
	indices[indexCount++] = j + 1;
	indices[indexCount++] = (j + 1) % segments + 1;
  }

  SDL_RenderGeometry(renderer, NULL, vertices, vertexCount, indices, indexCount);
}

void SDL_Clay_RenderWaveScreen(SDL_Renderer *renderer, float x, float y, float width, float height, const Clay_Color color, float *point_buffer, size_t buffer_len) {
  SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
  float samples_per_px = (float)buffer_len / width;

  for (int px = 0; px < (int)width; px++) {
	size_t start = (size_t)(px * samples_per_px);
	size_t end   = (size_t)((px + 1) * samples_per_px);
	if (end >= buffer_len) end = buffer_len - 1;

	float minv = 1.0f, maxv = -1.0f;
	for (size_t i = start; i <= end; i++) {
	  if (point_buffer[i] < minv) minv = point_buffer[i];
	  if (point_buffer[i] > maxv) maxv = point_buffer[i];
	}

	float y_min = y + height * (0.5f - 0.5f * maxv);
	float y_max = y + height * (0.5f - 0.5f * minv);

	SDL_RenderLine(renderer, x + px, y_min, x + px, y_max);
  }
}

SDL_Rect currentClippingRectangle;

void SDL_Clay_RenderClayCommands(Clay_SDL3RendererData *rendererData, Clay_RenderCommandArray *rcommands)
{
  for (int32_t i = 0; i < rcommands->length; i++) {
    Clay_RenderCommand *rcmd = Clay_RenderCommandArray_Get(rcommands, i);
    const Clay_BoundingBox bounding_box = rcmd->boundingBox;
    const SDL_FRect rect = { (int)bounding_box.x, (int)bounding_box.y, (int)bounding_box.width, (int)bounding_box.height };

    switch (rcmd->commandType) {
    case CLAY_RENDER_COMMAND_TYPE_RECTANGLE: {
      Clay_RectangleRenderData *config = &rcmd->renderData.rectangle;
      SDL_SetRenderDrawBlendMode(rendererData->renderer, SDL_BLENDMODE_BLEND);
      SDL_SetRenderDrawColor(rendererData->renderer, config->backgroundColor.r, config->backgroundColor.g, config->backgroundColor.b, config->backgroundColor.a);
      if (config->cornerRadius.topLeft > 0) {
        SDL_Clay_RenderFillRoundedRect(rendererData, rect, config->cornerRadius.topLeft, config->backgroundColor);
      } else {
        SDL_RenderFillRect(rendererData->renderer, &rect);
      }
    } break;
    case CLAY_RENDER_COMMAND_TYPE_TEXT: {
      Clay_TextRenderData *config = &rcmd->renderData.text;
      TTF_Font *font = rendererData->fonts[config->fontId];
      TTF_SetFontSize(font, config->fontSize);
      TTF_Text *text = TTF_CreateText(rendererData->textEngine, font, config->stringContents.chars, config->stringContents.length);
      TTF_SetTextColor(text, config->textColor.r, config->textColor.g, config->textColor.b, config->textColor.a);
      TTF_DrawRendererText(text, rect.x, rect.y);
      TTF_DestroyText(text);
    } break;
    case CLAY_RENDER_COMMAND_TYPE_BORDER: {
      Clay_BorderRenderData *config = &rcmd->renderData.border;

      const float minRadius = SDL_min(rect.w, rect.h) / 2.0f;
      const Clay_CornerRadius clampedRadii = {
        .topLeft = SDL_min(config->cornerRadius.topLeft, minRadius),
        .topRight = SDL_min(config->cornerRadius.topRight, minRadius),
        .bottomLeft = SDL_min(config->cornerRadius.bottomLeft, minRadius),
        .bottomRight = SDL_min(config->cornerRadius.bottomRight, minRadius)
      };
      //edges
      SDL_SetRenderDrawColor(rendererData->renderer, config->color.r, config->color.g, config->color.b, config->color.a);
      if (config->width.left > 0) {
        const float starting_y = rect.y + clampedRadii.topLeft;
        const float length = rect.h - clampedRadii.topLeft - clampedRadii.bottomLeft;
        SDL_FRect line = { rect.x - 1, starting_y, config->width.left, length };
        SDL_RenderFillRect(rendererData->renderer, &line);
      }
      if (config->width.right > 0) {
        const float starting_x = rect.x + rect.w - (float)config->width.right + 1;
        const float starting_y = rect.y + clampedRadii.topRight;
        const float length = rect.h - clampedRadii.topRight - clampedRadii.bottomRight;
        SDL_FRect line = { starting_x, starting_y, config->width.right, length };
        SDL_RenderFillRect(rendererData->renderer, &line);
      }
      if (config->width.top > 0) {
        const float starting_x = rect.x + clampedRadii.topLeft;
        const float length = rect.w - clampedRadii.topLeft - clampedRadii.topRight;
        SDL_FRect line = { starting_x, rect.y - 1, length, config->width.top };
        SDL_RenderFillRect(rendererData->renderer, &line);
      }
      if (config->width.bottom > 0) {
        const float starting_x = rect.x + clampedRadii.bottomLeft;
        const float starting_y = rect.y + rect.h - (float)config->width.bottom + 1;
        const float length = rect.w - clampedRadii.bottomLeft - clampedRadii.bottomRight;
        SDL_FRect line = { starting_x, starting_y, length, config->width.bottom };
        SDL_SetRenderDrawColor(rendererData->renderer, config->color.r, config->color.g, config->color.b, config->color.a);
        SDL_RenderFillRect(rendererData->renderer, &line);
      }
      //corners
      if (config->cornerRadius.topLeft > 0) {
        const float centerX = rect.x + clampedRadii.topLeft -1;
        const float centerY = rect.y + clampedRadii.topLeft - 1;
        SDL_Clay_RenderArc(rendererData, (SDL_FPoint){centerX, centerY}, clampedRadii.topLeft,
                           180.0f, 270.0f, config->width.top, config->color);
      }
      if (config->cornerRadius.topRight > 0) {
        const float centerX = rect.x + rect.w - clampedRadii.topRight;
        const float centerY = rect.y + clampedRadii.topRight - 1;
        SDL_Clay_RenderArc(rendererData, (SDL_FPoint){centerX, centerY}, clampedRadii.topRight,
                           270.0f, 360.0f, config->width.top, config->color);
      }
      if (config->cornerRadius.bottomLeft > 0) {
        const float centerX = rect.x + clampedRadii.bottomLeft -1;
        const float centerY = rect.y + rect.h - clampedRadii.bottomLeft;
        SDL_Clay_RenderArc(rendererData, (SDL_FPoint){centerX, centerY}, clampedRadii.bottomLeft,
                           90.0f, 180.0f, config->width.bottom, config->color);
      }
      if (config->cornerRadius.bottomRight > 0) {
        const float centerX = rect.x + rect.w - clampedRadii.bottomRight;
        const float centerY = rect.y + rect.h - clampedRadii.bottomRight;
        SDL_Clay_RenderArc(rendererData, (SDL_FPoint){centerX, centerY}, clampedRadii.bottomRight,
                           0.0f, 90.0f, config->width.bottom, config->color);
      }

    } break;
    case CLAY_RENDER_COMMAND_TYPE_SCISSOR_START: {
      Clay_BoundingBox boundingBox = rcmd->boundingBox;
      currentClippingRectangle = (SDL_Rect) {
        .x = boundingBox.x,
        .y = boundingBox.y,
        .w = boundingBox.width,
        .h = boundingBox.height,
      };
      SDL_SetRenderClipRect(rendererData->renderer, &currentClippingRectangle);
      break;
    }
    case CLAY_RENDER_COMMAND_TYPE_SCISSOR_END: {
      SDL_SetRenderClipRect(rendererData->renderer, NULL);
      break;
    }
    case CLAY_RENDER_COMMAND_TYPE_IMAGE: {
      SDL_Texture *texture = (SDL_Texture *)rcmd->renderData.image.imageData;
      const SDL_FRect dest = { rect.x, rect.y, rect.w, rect.h };
      SDL_RenderTexture(rendererData->renderer, texture, NULL, &dest);
      break;
    }

	case CLAY_RENDER_COMMAND_TYPE_CUSTOM: {
	  Clay_BoundingBox bounding_box = rcmd->boundingBox;

	  CustomElementData *custom_element = rcmd->renderData.custom.customData;
	  if (!custom_element) continue;

	  switch (custom_element->type) {
	  case CUSTOM_ELEMENT_TYPE_CIRCLE: {
		CircleData config = custom_element->circle;

		float start_angle = config.start_angle;
		float end_angle = config.value * 360 + start_angle;
		end_angle = end_angle > start_angle ? end_angle : end_angle + 360;
		
		SDL_Clay_RenderCircle(rendererData->renderer, bounding_box.x, bounding_box.y, bounding_box.width, bounding_box.height, start_angle, end_angle, config.color);
		break;
	  }
	  case CUSTOM_ELEMENT_TYPE_WAVE_SCREEN: {
		WaveScreenData config = custom_element->wave_screen;
		SDL_Clay_RenderWaveScreen(rendererData->renderer, bounding_box.x, bounding_box.y,
								  bounding_box.width, bounding_box.height, config.color,
								  config.point_buffer, config.buffer_len);
	  }
	  }
	  break; 
	}
    default:
      SDL_Log("Unknown render command type: %d", rcmd->commandType);
    }
  }
}
