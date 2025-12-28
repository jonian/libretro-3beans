#include <algorithm>

#include "screen_layout.h"

int ScreenLayout::screenArrangement = 0;
int ScreenLayout::screenSizing = 0;
int ScreenLayout::screenPosition = 0;
bool ScreenLayout::renderTopScreen = true;
bool ScreenLayout::renderBotScreen = true;

void ScreenLayout::update(bool swapScreens)
{
  bool singleScreen = screenArrangement == 2;

  bool alignCenter = screenPosition == 0;
  bool alignEnd = screenPosition == 2;

  bool scaleTop = !singleScreen && screenSizing == 1;
  bool scaleBot = !singleScreen && screenSizing == 2;

  int topScale = scaleTop ? 2 : 1;
  int botScale = scaleBot ? 2 : 1;

  renderTopScreen = !singleScreen || !swapScreens;
  renderBotScreen = !singleScreen || swapScreens;

  topWidth = 400 * topScale;
  topHeight = 240 * topScale;

  botWidth = 320 * botScale;
  botHeight = 240 * botScale;

  if (screenArrangement == 0)
  {
    minWidth = std::max(topWidth, botWidth);
    minHeight = topHeight + botHeight;

    topY = 0;
    botY = topHeight;

    if (alignCenter)
    {
      topX = (minWidth - topWidth) / 2;
      botX = (minWidth - botWidth) / 2;
    }
    else if (alignEnd)
    {
      topX = minWidth - topWidth;
      botX = minWidth - botWidth;
    }
    else
    {
      topX = botX = 0;
    }
  }
  else if (screenArrangement == 1)
  {
    minWidth = topWidth + botWidth;
    minHeight = std::max(topHeight, botHeight);

    topX = 0;
    botX = topWidth;

    if (alignCenter)
    {
      topY = (minHeight - topHeight) / 2;
      botY = (minHeight - botHeight) / 2;
    }
    else if (alignEnd)
    {
      topY = minHeight - topHeight;
      botY = minHeight - botHeight;
    }
    else
    {
      topY = botY = 0;
    }
  }
  else
  {
    topX = botX = 0;
    topY = botY = 0;

    if (renderBotScreen)
    {
      minWidth = botWidth;
      minHeight = botHeight;
    }
    else
    {
      minWidth = topWidth;
      minHeight = topHeight;
    }
  }
}

int ScreenLayout::getTouchX(int x, int y)
{
  return x - botX;
}

int ScreenLayout::getTouchY(int x, int y)
{
  return y - botY;
}
