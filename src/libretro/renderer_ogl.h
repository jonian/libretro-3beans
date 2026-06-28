#pragma once

#include <stdint.h>
#include <vector>

#include "glad/glad.h"
#include "renderer.h"

class RendererOgl: public VideoRenderer
{
public:
  RendererOgl() {};
  ~RendererOgl() {};

  void drawTopScreen(uint32_t *frame, ScreenLayout &layout);
  void drawBotScreen(uint32_t *frame, ScreenLayout &layout);
  void drawCursor(int x, int y, ScreenLayout &layout);
  void update(ScreenLayout &layout);
  void render(retro_video_refresh_t videoCallback, ScreenLayout &layout);
  void setFBO(uintptr_t fbo) { framebuffer = fbo; }
  void switchContext() { contextSwitched = !contextSwitched; }
  void resetContext();
  void destroyContext();

private:
  static const char *vtxCode;
  static const char *fragCode;

  std::vector<uint32_t> videoBuffer;
  uint32_t videoBufferSize;
  bool contextSwitched;

  GLuint framebuffer = 0;
  GLuint program = 0;
  GLuint vao = 0;
  GLuint vbo = 0;
  GLuint texture = 0;
  GLint winSizeLoc = 0;
};
