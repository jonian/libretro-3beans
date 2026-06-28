#pragma once

#include <stdint.h>
#include <vector>

#include "libretro.h"
#include "screen_layout.h"

class VideoRenderer
{
public:
  VideoRenderer() = default;
  virtual ~VideoRenderer() = default;

  void drawTopScreen(uint32_t *frame, ScreenLayout &layout);
  void drawBotScreen(uint32_t *frame, ScreenLayout &layout);
  void drawCursor(int x, int y, ScreenLayout &layout);
  void update(ScreenLayout &layout);

  virtual void render(retro_video_refresh_t videoCallback, ScreenLayout &layout) = 0;
  virtual void switchContext() {};
  virtual void resetContext() {};
  virtual void destroyContext() {};

protected:
  std::vector<uint32_t> videoBuffer;
  uint32_t videoBufferSize = 0;
};
