#pragma once

#include <stdint.h>
#include <cstring>
#include <string>
#include <regex>

inline int32_t clampValue(int32_t value, int32_t min, int32_t max)
{
  return std::max(min, std::min(max, value));
}

inline bool endsWith(std::string str, std::string end)
{
  return str.find(end, str.length() - end.length()) != std::string::npos;
}

inline std::string normalizePath(std::string path, bool addSlash = false)
{
  std::string newPath = path;
  if (addSlash && newPath.back() != '/') newPath += '/';
  if (!addSlash && newPath.back() == '/') newPath.erase(newPath.size() - 1);
#ifdef WINDOWS
  std::replace(newPath.begin(), newPath.end(), '\\', '/');
#endif
  return newPath;
}

inline uint32_t convertColor(uint32_t color)
{
  return 0xFF000000 |
    ((color & 0x0000FF) << 16) |
    ((color & 0x00FF00)) |
    ((color & 0xFF0000) >> 16);
}

inline void copyScreen(uint32_t *src, uint32_t *dst, int sw, int sh, int dx, int dy, int dw, int dh, int stride)
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
