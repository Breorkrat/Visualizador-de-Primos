module;
export module game_state;

export enum class MenuTab : int { Visuals, Colors, Simulation, HUD };

export enum class ColorMode : int {
  Calculated,
  Breathing,
  CustomStatic,
  CustomGradient
};

export struct VisualSettings {
  bool rippleEnabled = false;
  float rippleSpeed = 4.0f;
  float rippleIntensity = 0.08f;
  float breathSpeed = 50.0f;
  float pointSize = 2.5f;
  int webShapeSides = 0;
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
  ColorMode colorMode = ColorMode::Calculated;

  // HUD Visiblity
  bool showStats = true;
  bool showControls = false;
  bool showFPS = true;
  bool showCursor = false;
  bool drawAsWeb = false;

  // Slide menu
  bool showSideMenu = false;
  MenuTab activeMenuTab = MenuTab::Visuals;

  VisualSettings visuals;
  CameraState camera;

  char inputBuffer[16] = {0};
  bool isTyping = false;
};
