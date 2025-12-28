#include <cstdarg>
#include <algorithm>
#include <cstring>
#include <regex>
#include <chrono>

#include <fcntl.h>
#include <fstream>
#include <sstream>

#include "libretro.h"
#include "screen_layout.h"

#include "../core/settings.h"
#include "../core/core.h"
#include "../core/defines.h"

#ifndef VERSION
#define VERSION "0.1"
#endif

static retro_environment_t envCallback;
static retro_video_refresh_t videoCallback;
static retro_audio_sample_batch_t audioBatchCallback;
static retro_input_poll_t inputPollCallback;
static retro_input_state_t inputStateCallback;

static struct retro_log_callback logging;
static retro_log_printf_t logCallback;

static std::string systemPath;
static std::string savesPath;
static std::string romPath;

static Core *core;
static ScreenLayout layout;

static std::vector<uint32_t> videoBuffer;
static uint32_t videoBufferSize;

static std::string touchMode;
static std::string screenSwapMode;

static bool showTouchCursor;
static bool screenTouched;
static bool screenSwapped;
static bool swapScreens;

static auto cursorTimeout = 0;
static auto cursorMovedAt = std::chrono::steady_clock::now();
static bool cursorVisible = false;

static int lastMouseX = 0;
static int lastMouseY = 0;

static int touchX = 0;
static int touchY = 0;

static int keymap[] = {
  RETRO_DEVICE_ID_JOYPAD_A,
  RETRO_DEVICE_ID_JOYPAD_B,
  RETRO_DEVICE_ID_JOYPAD_SELECT,
  RETRO_DEVICE_ID_JOYPAD_START,
  RETRO_DEVICE_ID_JOYPAD_RIGHT,
  RETRO_DEVICE_ID_JOYPAD_LEFT,
  RETRO_DEVICE_ID_JOYPAD_UP,
  RETRO_DEVICE_ID_JOYPAD_DOWN,
  RETRO_DEVICE_ID_JOYPAD_R,
  RETRO_DEVICE_ID_JOYPAD_L,
  RETRO_DEVICE_ID_JOYPAD_X,
  RETRO_DEVICE_ID_JOYPAD_Y
};

static int32_t clampValue(int32_t value, int32_t min, int32_t max)
{
  return std::max(min, std::min(max, value));
}

static bool endsWith(std::string str, std::string end)
{
  return str.find(end, str.length() - end.length()) != std::string::npos;
}

static std::string normalizePath(std::string path, bool addSlash = false)
{
  std::string newPath = path;
  if (addSlash && newPath.back() != '/') newPath += '/';
  if (!addSlash && newPath.back() == '/') newPath.erase(newPath.size() - 1);
#ifdef WINDOWS
  std::replace(newPath.begin(), newPath.end(), '\\', '/');
#endif
  return newPath;
}

static void logFallback(enum retro_log_level level, const char *fmt, ...)
{
  (void)level;
  va_list va;
  va_start(va, fmt);
  vfprintf(stderr, fmt, va);
  va_end(va);
}

static std::string fetchVariable(std::string key, std::string def)
{
  struct retro_variable var = { nullptr };
  var.key = key.c_str();

  if (!envCallback(RETRO_ENVIRONMENT_GET_VARIABLE, &var) || var.value == nullptr)
  {
    logCallback(RETRO_LOG_WARN, "Fetching variable %s failed.", var.key);
    return def;
  }

  return std::string(var.value);
}

static bool fetchVariableBool(std::string key, bool def)
{
  return fetchVariable(key, def ? "enabled" : "disabled") == "enabled";
}

static int fetchVariableInt(std::string key, int def)
{
  std::string value = fetchVariable(key, std::to_string(def));

  if (!value.empty() && std::isdigit(value[0]))
    return std::stoi(value);

  return 0;
}

static int fetchVariableEnum(std::string key, std::vector<std::string> list, int def = 0)
{
  auto val = fetchVariable(key, list[def]);
  auto itr = std::find(list.begin(), list.end(), val);

  return std::distance(list.begin(), itr);
}

static std::string getSaveDir()
{
  char* dir = nullptr;
  if (!envCallback(RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY, &dir) || dir == nullptr)
  {
    logCallback(RETRO_LOG_INFO, "No save directory provided by LibRetro.");
    return std::string("3Beans");
  }
  return std::string(dir);
}

static std::string getSystemDir()
{
  char* dir = nullptr;
  if (!envCallback(RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY, &dir) || dir == nullptr)
  {
    logCallback(RETRO_LOG_INFO, "No system directory provided by LibRetro.");
    return std::string("3Beans");
  }
  return std::string(dir);
}

static bool getButtonState(unsigned id)
{
  return inputStateCallback(0, RETRO_DEVICE_JOYPAD, 0, id);
}

static float getAxisState(unsigned index, unsigned id)
{
  return inputStateCallback(0, RETRO_DEVICE_ANALOG, index, id);
}

static void initInput(void)
{
  static const struct retro_controller_description controllers[] = {
    { "Nintendo 3DS", RETRO_DEVICE_JOYPAD },
    { NULL, 0 },
  };

  static const struct retro_controller_info ports[] = {
    { controllers, 1 },
    { NULL, 0 },
  };

  envCallback(RETRO_ENVIRONMENT_SET_CONTROLLER_INFO, (void*)ports);

  struct retro_input_descriptor desc[] = {
    { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT, "Left" },
    { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP, "Up" },
    { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN, "Down" },
    { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT, "Right" },
    { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A, "A" },
    { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B, "B" },
    { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_SELECT, "Select" },
    { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_START, "Start" },
    { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R, "R" },
    { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L, "L" },
    { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_X, "X" },
    { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_Y, "Y" },
    { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R2, "Swap Screens" },
    { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R3, "Screen Touch" },
    { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L3, "Home Button" },
    { 0, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_X, "Circle Pad X" },
    { 0, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_Y, "Circle Pad Y" },
    { 0, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_RIGHT, RETRO_DEVICE_ID_ANALOG_X, "Pointer X" },
    { 0, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_RIGHT, RETRO_DEVICE_ID_ANALOG_Y, "Pointer Y" },
    { 0 },
  };

  envCallback(RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS, &desc);
}

static void initConfig()
{
  static const retro_variable values[] = {
    { "3beans_cartAutoBoot", "Cart Auto Boot; enabled|disabled" },
    { "3beans_fpsLimiter", "FPS Limiter; enabled|disabled" },
    { "3beans_threadedGpu", "Threaded GPU; disabled|enabled" },
    { "3beans_screenArrangement", "Screen Arrangement; Vertical|Horizontal|Single Screen" },
    { "3beans_screenSizing", "Screen Sizing; Default|Enlarge Top|Enlarge Bottom" },
    { "3beans_screenPosition", "Screen Position; Center|Start|End" },
    { "3beans_swapScreenMode", "Swap Screen Mode; Toggle|Hold" },
    { "3beans_touchMode", "Touch Mode; Auto|Pointer|Joystick|None" },
    { "3beans_touchCursor", "Show Touch Cursor; enabled|disabled" },
    { "3beans_cursorTimeout", "Hide Cursor Timeout; 3 Seconds|5 Seconds|10 Seconds|15 Seconds|20 Seconds|Never Hide" },
    { nullptr, nullptr }
  };

  envCallback(RETRO_ENVIRONMENT_SET_VARIABLES, (void*)values);
}

static void updateConfig()
{
  Settings::basePath = normalizePath(savesPath, false);
  Settings::boot11Path = systemPath + "boot11.bin";
  Settings::boot9Path = systemPath + "boot9.bin";
  Settings::nandPath = systemPath + "nand.bin";
  Settings::sdPath = systemPath + "3ds_sd_card.img";

  Settings::cartAutoBoot = fetchVariableBool("3beans_cartAutoBoot", true);
  Settings::fpsLimiter = fetchVariableBool("3beans_fpsLimiter", true);
  Settings::threadedGpu = fetchVariableBool("3beans_threadedGpu", false);

  ScreenLayout::screenArrangement = fetchVariableEnum("3beans_screenArrangement", {"Vertical", "Horizontal", "Single Screen"});
  ScreenLayout::screenSizing = fetchVariableEnum("3beans_screenSizing", {"Default", "Enlarge Top", "Enlarge Bottom"});
  ScreenLayout::screenPosition = fetchVariableEnum("3beans_screenPosition", {"Center", "Start", "End"});

  screenSwapMode = fetchVariable("3beans_swapScreenMode", "Toggle");
  touchMode = fetchVariable("3beans_touchMode", "Touch");
  showTouchCursor = fetchVariableBool("3beans_touchCursor", true);
  cursorTimeout = fetchVariableInt("3beans_cursorTimeout", 3);
}

static void updateScreen()
{
  layout.update(swapScreens);

  auto bsize = layout.minWidth * layout.minHeight;

  if (videoBufferSize != bsize)
  {
    videoBuffer.resize(bsize);
    videoBufferSize = bsize;
  }

  memset(videoBuffer.data(), 0, videoBuffer.size() * sizeof(videoBuffer[0]));

  retro_system_av_info info;
  retro_get_system_av_info(&info);
  envCallback(RETRO_ENVIRONMENT_SET_GEOMETRY, &info);
}

static void checkConfigVariables()
{
  bool updated = false;
  envCallback(RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE, &updated);

  if (updated)
  {
    updateConfig();
    updateScreen();
  }
}

static void drawCursor(uint32_t *data, int32_t pointX, int32_t pointY, int32_t size = 3)
{
  auto scale = layout.botWidth / 320;

  uint32_t posX = clampValue(pointX, size, (layout.botWidth / scale) - size);
  uint32_t posY = clampValue(pointY, size, (layout.botHeight / scale) - size);

  uint32_t minX = layout.botX;
  uint32_t maxX = layout.minWidth;

  uint32_t minY = layout.botY;
  uint32_t maxY = layout.minHeight;

  uint32_t curX = layout.botX + (posX * scale);
  uint32_t curY = layout.botY + (posY * scale);

  uint32_t cursorSize = size * scale;

  uint32_t startY = clampValue(curY - cursorSize, minY, maxY);
  uint32_t endY = clampValue(curY + cursorSize, minY, maxY);

  uint32_t startX = clampValue(curX - cursorSize, minX, maxX);
  uint32_t endX = clampValue(curX + cursorSize, minX, maxX);

  for (uint32_t y = startY; y < endY; y++)
  {
    for (uint32_t x = startX; x < endX; x++)
    {
      uint32_t& pixel = data[(y * maxX) + x];
      pixel = (0xFFFFFF - pixel) | 0xFF000000;
    }
  }
}

static void copyScreen(uint32_t *src, uint32_t *dst, int sw, int sh, int dx, int dy, int dw, int dh, int stride)
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

static inline uint32_t convertColor(uint32_t color)
{
  return 0xFF000000 |
    ((color & 0x0000FF) << 16) |
    ((color & 0x00FF00)) |
    ((color & 0xFF0000) >> 16);
}

static void renderVideo()
{
  static uint32_t bufferTop[400 * 240];
  static uint32_t bufferBot[320 * 240];

  if (uint32_t *frame = core->pdc.getFrame())
  {
    if (ScreenLayout::renderTopScreen)
    {
      for (int i = 0; i < 400 * 240; i++)
        bufferTop[i] = convertColor(frame[i]);

      copyScreen(
        bufferTop, videoBuffer.data(),
        400, 240,
        layout.topX, layout.topY,
        layout.topWidth, layout.topHeight,
        layout.minWidth
      );
    }

    if (ScreenLayout::renderBotScreen)
    {
      for (int y = 0; y < 240; y++)
        for (int x = 0; x < 320; x++)
          bufferBot[y * 320 + x] = convertColor(frame[(y + 240) * 400 + (x + 40)]);

      copyScreen(
        bufferBot, videoBuffer.data(),
        320, 240,
        layout.botX, layout.botY,
        layout.botWidth, layout.botHeight,
        layout.minWidth
      );

      if (showTouchCursor && cursorVisible)
        drawCursor(videoBuffer.data(), touchX, touchY);
    }

    delete[] frame;
  }

  uint32_t stride = layout.minWidth * 4;
  videoCallback(videoBuffer.data(), layout.minWidth, layout.minHeight, stride);
}

static void renderAudio()
{
  static int16_t buffer[1024 * 2];

  if (uint32_t *samples = core->csnd.getSamples(48000, 1024))
  {
    for (int i = 0; i < 1024; i++)
    {
      buffer[i * 2 + 0] = samples[i] >>  0;
      buffer[i * 2 + 1] = samples[i] >> 16;
    }
  }

  uint32_t size = sizeof(buffer) / (2 * sizeof(int16_t));
  audioBatchCallback(buffer, size);
}

static void updateCursorState()
{
  if (showTouchCursor && cursorTimeout)
  {
    if (cursorVisible)
    {
      auto current = std::chrono::steady_clock::now();
      auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(current - cursorMovedAt).count();

      if (elapsed >= cursorTimeout) cursorVisible = false;
    }
  }
  else
  {
    cursorVisible = true;
  }
}

static bool createCore(std::string cartPath = "")
{
  try
  {
    if (core) delete core;

    core = new Core(cartPath, nullptr);
    return true;
  }
  catch (CoreError e)
  {
    logCallback(RETRO_LOG_INFO, "Error Loading Boot ROMs and/or NAND dump");

    core = nullptr;
    return false;
  }
}

void retro_get_system_info(retro_system_info* info)
{
  info->need_fullpath = true;
  info->valid_extensions = "3ds|cci";
  info->library_version = VERSION;
  info->library_name = "3Beans";
  info->block_extract = false;
}

void retro_get_system_av_info(retro_system_av_info* info)
{
  info->geometry.base_width = layout.minWidth;
  info->geometry.base_height = layout.minHeight;

  info->geometry.max_width = info->geometry.base_width;
  info->geometry.max_height = info->geometry.base_height;
  info->geometry.aspect_ratio = (float)layout.minWidth / (float)layout.minHeight;

  info->timing.fps = 60.0f;
  info->timing.sample_rate = 32.0f * 1024.0f;
}

void retro_set_environment(retro_environment_t cb)
{
  bool nogameSupport = true;
  cb(RETRO_ENVIRONMENT_SET_SUPPORT_NO_GAME, &nogameSupport);

  envCallback = cb;
}

void retro_set_video_refresh(retro_video_refresh_t cb)
{
  videoCallback = cb;
}

void retro_set_audio_sample_batch(retro_audio_sample_batch_t cb)
{
  audioBatchCallback = cb;
}

void retro_set_audio_sample(retro_audio_sample_t cb)
{
}

void retro_set_input_poll(retro_input_poll_t cb)
{
  inputPollCallback = cb;
}

void retro_set_input_state(retro_input_state_t cb)
{
  inputStateCallback = cb;
}

void retro_init(void)
{
  enum retro_pixel_format xrgb888 = RETRO_PIXEL_FORMAT_XRGB8888;
  envCallback(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &xrgb888);

  if (envCallback(RETRO_ENVIRONMENT_GET_LOG_INTERFACE, &logging))
    logCallback = logging.log;
  else
    logCallback = logFallback;

  systemPath = normalizePath(getSystemDir(), true);
  savesPath = normalizePath(getSaveDir(), true);
}

void retro_deinit(void)
{
  logCallback = nullptr;
}

bool retro_load_game(const struct retro_game_info* info)
{
  romPath = "";

  if (info && info->path)
    romPath = normalizePath(info->path);

  initConfig();
  updateConfig();

  initInput();
  updateScreen();

  return createCore(romPath);
}

bool retro_load_game_special(unsigned type, const struct retro_game_info* info, size_t size)
{
  return false;
}

void retro_unload_game(void)
{
  if (core) delete core;
}

void retro_reset(void)
{
  createCore(romPath);
}

void retro_run(void)
{
  checkConfigVariables();
  updateCursorState();
  inputPollCallback();

  for (int i = 0; i < sizeof(keymap) / sizeof(*keymap); ++i)
  {
    if (getButtonState(keymap[i]))
      core->input.pressKey(i);
    else
      core->input.releaseKey(i);
  }

  if (getButtonState(RETRO_DEVICE_ID_JOYPAD_L3))
    core->input.pressHome();
  else
    core->input.releaseHome();

  float xLeft = getAxisState(RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_X);
  float yLeft = getAxisState(RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_Y);

  int stickX = (xLeft / +32767) * 0x9C;
  int stickY = (yLeft / -32767) * 0x9C;

  core->input.setLStick(stickX, stickY);

  if (ScreenLayout::screenArrangement == 2)
  {
    bool swapPressed = getButtonState(RETRO_DEVICE_ID_JOYPAD_R2);

    if (screenSwapped != swapPressed)
    {
      bool prevSwap = swapScreens;

      if (screenSwapMode == "Toggle" && swapPressed)
        swapScreens = !swapScreens;

      if (screenSwapMode == "Hold")
        swapScreens = swapPressed;

      if (prevSwap != swapScreens)
        updateScreen();

      screenSwapped = swapPressed;
    }
  }

  if (ScreenLayout::renderBotScreen)
  {
    bool touchScreen = false;
    auto pointerX = touchX;
    auto pointerY = touchY;

    if (touchMode == "Pointer" || touchMode == "Auto")
    {
      auto posX = inputStateCallback(0, RETRO_DEVICE_POINTER, 0, RETRO_DEVICE_ID_POINTER_X);
      auto posY = inputStateCallback(0, RETRO_DEVICE_POINTER, 0, RETRO_DEVICE_ID_POINTER_Y);

      auto newX = static_cast<int>((posX + 0x7fff) / (float)(0x7fff * 2) * layout.minWidth);
      auto newY = static_cast<int>((posY + 0x7fff) / (float)(0x7fff * 2) * layout.minHeight);

      bool inScreenX = newX >= layout.botX && newX <= layout.botX + layout.botWidth;
      bool inScreenY = newY >= layout.botY && newY <= layout.botY + layout.botHeight;

      if (inScreenX && inScreenY)
      {
        touchScreen |= inputStateCallback(0, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_LEFT);
        touchScreen |= inputStateCallback(0, RETRO_DEVICE_POINTER, 0, RETRO_DEVICE_ID_POINTER_PRESSED);
      }

      if ((posX != 0 || posY != 0) && (lastMouseX != newX || lastMouseY != newY))
      {
        lastMouseX = newX;
        lastMouseY = newY;

        pointerX = layout.getTouchX(newX, newY);
        pointerY = layout.getTouchY(newX, newY);
      }
    }

    if (touchMode == "Joystick" || touchMode == "Auto")
    {
      auto speedX = (layout.botWidth / 60.0);
      auto speedY = (layout.botHeight / 60.0);

      float moveX = getAxisState(RETRO_DEVICE_INDEX_ANALOG_RIGHT, RETRO_DEVICE_ID_ANALOG_X);
      float moveY = getAxisState(RETRO_DEVICE_INDEX_ANALOG_RIGHT, RETRO_DEVICE_ID_ANALOG_Y);

      touchScreen |= getButtonState(RETRO_DEVICE_ID_JOYPAD_R3);

      if (moveX != 0 || moveY != 0)
      {
        pointerX += static_cast<int>((moveX / 32767) * speedX);
        pointerY += static_cast<int>((moveY / 32767) * speedY);
      }
    }

    if (cursorTimeout && (pointerX != touchX || pointerY != touchY))
    {
      cursorVisible = true;
      cursorMovedAt = std::chrono::steady_clock::now();
    }

    touchX = clampValue(pointerX, 0, layout.botWidth);
    touchY = clampValue(pointerY, 0, layout.botHeight);

    if (touchScreen)
    {
      core->input.pressScreen(touchX, touchY);
      screenTouched = true;
    }
    else if (screenTouched)
    {
      core->input.releaseScreen();
      screenTouched = false;
    }
  }

  core->runFrame();

  renderVideo();
  renderAudio();
}

void retro_set_controller_port_device(unsigned port, unsigned device)
{
}

size_t retro_serialize_size(void)
{
  return 0;
}

bool retro_serialize(void* data, size_t size)
{
  return false;
}

bool retro_unserialize(const void* data, size_t size)
{
  return false;
}

unsigned retro_get_region(void)
{
  return RETRO_REGION_NTSC;
}

unsigned retro_api_version()
{
  return RETRO_API_VERSION;
}

size_t retro_get_memory_size(unsigned id)
{
  if (id == RETRO_MEMORY_SYSTEM_RAM)
  {
    return 0x600000;
  }
  return 0;
}

void* retro_get_memory_data(unsigned id)
{
  if (id == RETRO_MEMORY_SYSTEM_RAM)
  {
    return core->memory.getRam();
  }
  return NULL;
}

void retro_cheat_set(unsigned index, bool enabled, const char* code)
{
}

void retro_cheat_reset(void)
{
}
