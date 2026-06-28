#include <cstring>

#include "renderer.h"
#include "utils.h"

void VideoRenderer::update(ScreenLayout &layout)
{
  auto bsize = layout.minWidth * layout.minHeight;

  if (videoBufferSize != bsize)
  {
    videoBuffer.resize(bsize);
    videoBufferSize = bsize;
  }

  memset(videoBuffer.data(), 0, videoBuffer.size() * sizeof(videoBuffer[0]));
}

void VideoRenderer::drawTopScreen(uint32_t *frame, ScreenLayout &layout)
{
  copyScreen(
    frame, videoBuffer.data(),
    400, 240, 400,
    layout.topX, layout.topY,
    layout.topWidth, layout.topHeight,
    layout.minWidth
  );
}

void VideoRenderer::drawBotScreen(uint32_t *frame, ScreenLayout &layout)
{
  copyScreen(
    frame + 400 * 240 + 40, videoBuffer.data(),
    320, 240, 400,
    layout.botX, layout.botY,
    layout.botWidth, layout.botHeight,
    layout.minWidth
  );
}

void VideoRenderer::drawCursor(int x, int y, ScreenLayout &layout)
{
  drawPointer(
    videoBuffer.data(), x, y,
    layout.botX, layout.botY,
    layout.botWidth, layout.botHeight,
    layout.minWidth, layout.botWidth / 320
  );
}
