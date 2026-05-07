module;
export module game_state;

export enum class ColorMode {
  Calculated,
  Breathing,
  CustomStatic,
  CustomGradient
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
  int colorPickerVisible = 0; // 0 = Hidden, 1 = Static, 2 = Gradient
};
