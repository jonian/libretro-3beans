#pragma once

#include <stdint.h>

#include "renderer.h"

class RendererSoft: public VideoRenderer
{
public:
  RendererSoft() {};
  ~RendererSoft() {};

  void render(retro_video_refresh_t videoCallback, ScreenLayout &layout);
};
