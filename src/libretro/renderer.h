#pragma once

#include <stdint.h>

#include "libretro.h"
#include "screen_layout.h"

class VideoRenderer
{
public:
  VideoRenderer() = default;
  virtual ~VideoRenderer() = default;

  virtual void drawTopScreen(uint32_t *frame, ScreenLayout &layout) = 0;
  virtual void drawBotScreen(uint32_t *frame, ScreenLayout &layout) = 0;
  virtual void drawCursor(int x, int y, ScreenLayout &layout) = 0;
  virtual void update(ScreenLayout &layout) = 0;
  virtual void render(retro_video_refresh_t videoCallback, ScreenLayout &layout) = 0;
};
