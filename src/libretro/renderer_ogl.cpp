#include <cstring>

#include "renderer_ogl.h"
#include "utils.h"

struct CanvasVtx {
  float x, y;
  float s, t;
};

const char *RendererOgl::vtxCode = R"(
  #version 330

  in vec2 inPosition;
  in vec2 inTexCoord;
  out vec2 vtxTexCoord;
  uniform vec2 winSize;

  void main() {
    gl_Position = vec4(inPosition.x / winSize.x * 2 - 1, inPosition.y / winSize.y * -2 + 1, 0, 1);
    vtxTexCoord = inTexCoord;
  }
)";

const char *RendererOgl::fragCode = R"(
  #version 330

  in vec2 vtxTexCoord;
  out vec4 fragColor;
  uniform sampler2D texUnit;

  void main() {
    fragColor = texture(texUnit, vtxTexCoord);
  }
)";

void RendererOgl::resetContext()
{
  GLint vtxShader = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vtxShader, 1, &vtxCode, nullptr);
  glCompileShader(vtxShader);
  GLint fragShader = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fragShader, 1, &fragCode, nullptr);
  glCompileShader(fragShader);

  program = glCreateProgram();
  glAttachShader(program, vtxShader);
  glAttachShader(program, fragShader);
  glLinkProgram(program);
  glUseProgram(program);
  glDeleteShader(vtxShader);
  glDeleteShader(fragShader);

  glGenVertexArrays(1, &vao);
  glBindVertexArray(vao);
  glGenBuffers(1, &vbo);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  GLint loc = glGetAttribLocation(program, "inPosition");
  glVertexAttribPointer(loc, 2, GL_FLOAT, GL_FALSE, sizeof(CanvasVtx), (void*)offsetof(CanvasVtx, x));
  glEnableVertexAttribArray(loc);
  loc = glGetAttribLocation(program, "inTexCoord");
  glVertexAttribPointer(loc, 2, GL_FLOAT, GL_FALSE, sizeof(CanvasVtx), (void*)offsetof(CanvasVtx, s));
  glEnableVertexAttribArray(loc);

  glEnable(GL_TEXTURE_2D);
  glGenTextures(1, &texture);
  glBindTexture(GL_TEXTURE_2D, texture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  winSizeLoc = glGetUniformLocation(program, "winSize");
}

void RendererOgl::destroyContext()
{
  glDeleteTextures(1, &texture);
  glDeleteBuffers(1, &vbo);
  glDeleteVertexArrays(1, &vao);
  glDeleteProgram(program);

  texture = 0;
  vao = 0;
  vbo = 0;
  program = 0;
}

void RendererOgl::render(retro_video_refresh_t videoCallback, ScreenLayout &layout)
{
  glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
  glViewport(0, 0, layout.minWidth, layout.minHeight);

  glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT);

  glUseProgram(program);
  glUniform2f(winSizeLoc, (float)layout.minWidth, (float)layout.minHeight);

  glBindVertexArray(vao);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, texture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, layout.minWidth, layout.minHeight, 0, GL_BGRA, GL_UNSIGNED_BYTE, videoBuffer.data());

  CanvasVtx vertices[] = {
    { 0.0, 0.0, 0.0f, 0.0f },
    { (float)layout.minWidth, 0.0, 1.0f, 0.0f },
    { (float)layout.minWidth, (float)layout.minHeight, 1.0f, 1.0f },
    { 0.0, (float)layout.minHeight, 0.0f, 1.0f }
  };

  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);
  glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

  videoCallback(RETRO_HW_FRAME_BUFFER_VALID, layout.minWidth, layout.minHeight, 0);
}

void RendererOgl::update(ScreenLayout &layout)
{
  auto bsize = layout.minWidth * layout.minHeight;

  if (videoBufferSize != bsize)
  {
    videoBuffer.resize(bsize);
    videoBufferSize = bsize;
  }

  memset(videoBuffer.data(), 0, videoBuffer.size() * sizeof(videoBuffer[0]));
}

void RendererOgl::drawTopScreen(uint32_t *frame, ScreenLayout &layout)
{
  copyScreen(
    frame, videoBuffer.data(),
    400, 240, 400,
    layout.topX, layout.topY,
    layout.topWidth, layout.topHeight,
    layout.minWidth
  );
}

void RendererOgl::drawBotScreen(uint32_t *frame, ScreenLayout &layout)
{
  copyScreen(
    frame + 400 * 240 + 40, videoBuffer.data(),
    320, 240, 400,
    layout.botX, layout.botY,
    layout.botWidth, layout.botHeight,
    layout.minWidth
  );
}

void RendererOgl::drawCursor(int32_t pointX, int32_t pointY, ScreenLayout &layout)
{
  drawPointer(
    videoBuffer.data(), pointX, pointY,
    layout.botX, layout.botY,
    layout.botWidth, layout.botHeight,
    layout.minWidth, layout.botWidth / 320
  );
}
