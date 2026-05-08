module;
export module game_state;

export enum class ColorMode {
  Calculated,
  Breathing,
  CustomStatic,
  CustomGradient
};

export struct CameraState {
  float x = 0.0f;
  float y = 0.0f;
  float zoom = 1.0f;
  char zoomMode = 0; // 0: Cover, 1: Full, 2: Free
};

export struct GameState {
  float currentLimit = 0;
  float primesPerSecond = 500.0f;
  bool ppsLock = false;
  bool benchmarkMode = false;

  unsigned int divMode = 0;
  ColorMode colorMode = ColorMode::Breathing;

  bool showStats = true;
  bool showControls = true;
  bool showFPS = false;
  bool showCursor = false;
  bool drawAsWeb = false;
  int colorPickerVisible = 0; // 0 = Hidden, 1 = Solid, 2 = Gradient

  CameraState camera;

  char inputBuffer[16] = {0};
  bool isTyping = false;
};
