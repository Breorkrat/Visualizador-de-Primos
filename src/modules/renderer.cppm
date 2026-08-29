module;
#include <string>
#include <vector>
export module renderer;

import game_state;
import math_logic;

export class Renderer {
public:
  Renderer(int width, int height, const std::string &title);
  ~Renderer();

  // Window lifecycle
  bool WindowShouldClose() const;
  float GetDeltaTime() const;
  void BeginFrame();
  void EndFrame();

  // Sends points to GPU VRAM
  void SyncGPUData(const std::vector<NumberPoint> &points, bool fullReset);

  // Isolated methods
  void DrawUI(const std::vector<NumberPoint> &points, const GameState &state);
  void DrawScene(const std::vector<NumberPoint> &points, GameState &state);

private:
  struct Impl; // PIMPL to hide raylib
  Impl *impl;
};
