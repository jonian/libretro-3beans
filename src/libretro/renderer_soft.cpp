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
    400, 240,
    layout.topX, layout.topY,
    layout.topWidth, layout.topHeight,
    layout.minWidth
  );
}

void RendererSoft::drawBotScreen(uint32_t *frame, ScreenLayout &layout)
{
  copyScreen(
    frame, videoBuffer.data(),
    320, 240,
    layout.botX, layout.botY,
    layout.botWidth, layout.botHeight,
    layout.minWidth
  );
}

void RendererSoft::drawCursor(int32_t pointX, int32_t pointY, ScreenLayout &layout)
{
  uint32_t *data = videoBuffer.data();

  int32_t size = 3;
  int32_t scale = layout.botWidth / 320;

  uint32_t posX = clampValue(pointX, size, (layout.botWidth / scale) - size);
  uint32_t posY = clampValue(pointY, size, (layout.botHeight / scale) - size);

  uint32_t minX = layout.botX;
  uint32_t maxX = layout.minWidth;

  uint32_t minY = layout.botY;
  uint32_t maxY = layout.minHeight;

  uint32_t curX = layout.botX + (posX * scale);
  uint32_t curY = layout.botY + (posY * scale);

  uint32_t cursorSize = size * scale;

  uint32_t startY = clampValue(curY - cursorSize, minY, maxY);
  uint32_t endY = clampValue(curY + cursorSize, minY, maxY);

  uint32_t startX = clampValue(curX - cursorSize, minX, maxX);
  uint32_t endX = clampValue(curX + cursorSize, minX, maxX);

  for (uint32_t y = startY; y < endY; y++)
  {
    for (uint32_t x = startX; x < endX; x++)
    {
      uint32_t& pixel = data[(y * maxX) + x];
      pixel = (0xFFFFFF - pixel) | 0xFF000000;
    }
  }
}

void RendererSoft::copyScreen(uint32_t *src, uint32_t *dst, int sw, int sh, int dx, int dy, int dw, int dh, int stride)
{
  int scaleX = dw / sw;
  int scaleY = dh / sh;

  if ((scaleX >= 1 && scaleY >= 1) && (scaleX > 1 || scaleY > 1))
  {
    for (int y = 0; y < dh; ++y)
    {
      int srcY = (y / scaleY) * sw;
      int dstY = (dy + y) * stride + dx;

      for (int x = 0; x < dw; ++x)
        dst[dstY + x] = src[srcY + (x / scaleX)];
    }
  }
  else if (dx == 0 && dw == stride)
  {
    int pixels = dw * dh * sizeof(uint32_t);
    int offset = dy * stride + dx;

    memcpy(dst + offset, src, pixels);
  }
  else
  {
    int rowSize = dw * sizeof(uint32_t);

    for (int y = 0; y < dh; ++y)
    {
      int srcY = y * sw;
      int dstY = (dy + y) * stride + dx;

      memcpy(dst + dstY, src + srcY, rowSize);
    }
  }
}
