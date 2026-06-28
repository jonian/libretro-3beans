#include <cstring>

#include "renderer_soft.h"
#include "utils.h"

void RendererSoft::render(retro_video_refresh_t videoCallback, ScreenLayout &layout)
{
  uint32_t stride = layout.minWidth * 4;
  videoCallback(videoBuffer.data(), layout.minWidth, layout.minHeight, stride);
}

void RendererSoft::update(ScreenLayout &layout)
{
  auto bsize = layout.minWidth * layout.minHeight;

  if (videoBufferSize != bsize)
  {
    videoBuffer.resize(bsize);
    videoBufferSize = bsize;
  }

  memset(videoBuffer.data(), 0, videoBuffer.size() * sizeof(videoBuffer[0]));
}

void RendererSoft::drawTopScreen(uint32_t *frame, ScreenLayout &layout)
{
  copyScreen(
    frame, videoBuffer.data(),
    400, 240, 400,
    layout.topX, layout.topY,
    layout.topWidth, layout.topHeight,
    layout.minWidth
  );
}

void RendererSoft::drawBotScreen(uint32_t *frame, ScreenLayout &layout)
{
  copyScreen(
    frame + 400 * 240 + 40, videoBuffer.data(),
    320, 240, 400,
    layout.botX, layout.botY,
    layout.botWidth, layout.botHeight,
    layout.minWidth
  );
}

void RendererSoft::drawCursor(int32_t pointX, int32_t pointY, ScreenLayout &layout)
{
  drawPointer(
    videoBuffer.data(), pointX, pointY,
    layout.botX, layout.botY,
    layout.botWidth, layout.botHeight,
    layout.minWidth, layout.botWidth / 320
  );
}
