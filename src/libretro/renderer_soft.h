#pragma once

#include <stdint.h>
#include <vector>

#include "renderer.h"

class RendererSoft: public VideoRenderer
{
public:
  RendererSoft() {};
  ~RendererSoft() {};

  void drawTopScreen(uint32_t *frame, ScreenLayout &layout);
  void drawBotScreen(uint32_t *frame, ScreenLayout &layout);
  void drawCursor(int x, int y, ScreenLayout &layout);
  void update(ScreenLayout &layout);
  void render(retro_video_refresh_t videoCallback, ScreenLayout &layout);

private:
  void copyScreen(uint32_t *src, uint32_t *dst, int sw, int sh, int dx, int dy, int dw, int dh, int stride);

  std::vector<uint32_t> videoBuffer;
  uint32_t videoBufferSize;
};
