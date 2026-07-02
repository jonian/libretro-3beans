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

inline void copyScreen(uint32_t *src, uint32_t *dst, int sw, int sh, int ss, int dx, int dy, int dw, int dh, int ds)
{
  int scaleX = dw / sw;
  int scaleY = dh / sh;

  if ((scaleX >= 1 && scaleY >= 1) && (scaleX > 1 || scaleY > 1))
  {
    int rowBytes = dw * sizeof(uint32_t);

    for (int y = 0; y < sh; ++y)
    {
      uint32_t *srcRow = src + y * ss;
      uint32_t *dstRow = dst + (dy + y * scaleY) * ds + dx;
      uint32_t *newRow = dstRow;

      for (int x = 0; x < sw; ++x)
      {
        for (int cx = 0; cx < scaleX; ++cx)
          *newRow++ = srcRow[x];
      }

      for (int cy = 1; cy < scaleY; ++cy)
        memcpy(dstRow + cy * ds, dstRow, rowBytes);
    }
  }
  else if (dx == 0 && dw == ds && sw == ss)
  {
    int pixels = dw * dh * sizeof(uint32_t);
    int offset = dy * ds + dx;

    memcpy(dst + offset, src, pixels);
  }
  else
  {
    int rowSize = dw * sizeof(uint32_t);

    for (int y = 0; y < dh; ++y)
    {
      int srcY = y * ss;
      int dstY = (dy + y) * ds + dx;

      memcpy(dst + dstY, src + srcY, rowSize);
    }
  }
}

inline void drawPointer(uint32_t *dst, int x, int y, int dx, int dy, int dw, int dh, int ds, int scale = 1, int size = 3)
{
  int posX = clampValue(x, size, (dw / scale) - size);
  int posY = clampValue(y, size, (dh / scale) - size);

  int curX = dx + (posX * scale);
  int curY = dy + (posY * scale);

  int startY = curY - (size * scale);
  int endY = curY + (size * scale);

  int startX = curX - (size * scale);
  int endX = curX + (size * scale);

  for (int py = startY; py < endY; py++)
  {
    for (int px = startX; px < endX; px++)
    {
      uint32_t& pixel = dst[(py * ds) + px];
      pixel = (0xFFFFFF - pixel) | 0xFF000000;
    }
  }
}
