#pragma once

#include <stdint.h>

#include "glad/glad.h"
#include "renderer.h"

class RendererOgl: public VideoRenderer
{
public:
  RendererOgl() {};
  ~RendererOgl() {};

  void render(retro_video_refresh_t videoCallback, ScreenLayout &layout);
  void setFBO(uintptr_t fbo) { framebuffer = fbo; }
  void resetContext();
  void destroyContext();

private:
  static const char *vtxCode;
  static const char *fragCode;

  GLuint framebuffer = 0;
  GLuint program = 0;
  GLuint vao = 0;
  GLuint vbo = 0;
  GLuint texture = 0;
  GLint winSizeLoc = 0;
};
