# 0 "/home/breorkrat/Documents/projetos/Prime-Visualizer/src/modules/game_state.cppm"
# 0 "<built-in>"
# 0 "<command-line>"
# 1 "/usr/include/stdc-predef.h" 1 3
# 0 "<command-line>" 2
# 1 "/home/breorkrat/Documents/projetos/Prime-Visualizer/src/modules/game_state.cppm"
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
  int colorPickerVisible = 0;
};
