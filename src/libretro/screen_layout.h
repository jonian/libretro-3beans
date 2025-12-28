#pragma once

class ScreenLayout
{
public:
  static int screenArrangement;
  static int screenSizing;
  static int screenPosition;
  static bool renderTopScreen;
  static bool renderBotScreen;

  int minWidth = 0, minHeight = 0;
  int topX = 0, botX = 0;
  int topY = 0, botY = 0;
  int topWidth = 0, botWidth = 0;
  int topHeight = 0, botHeight = 0;

  void update(bool swapScreens);

  int getTouchX(int x, int y);
  int getTouchY(int x, int y);
};
