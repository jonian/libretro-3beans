#include "renderer_soft.h"

void RendererSoft::render(retro_video_refresh_t videoCallback, ScreenLayout &layout)
{
  uint32_t stride = layout.minWidth * 4;
  videoCallback(videoBuffer.data(), layout.minWidth, layout.minHeight, stride);
}
