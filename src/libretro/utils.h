#pragma once

#include <stdint.h>
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
