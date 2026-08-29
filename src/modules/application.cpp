module;
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <raylib.h>

module application;

import input;

constexpr int THRESHOLD = 1000;
constexpr int SEARCH_RANGE = 500000;
constexpr int FIRST_GEN = 1000;
constexpr int WINDOW_WIDTH = 1920;
constexpr int WINDOW_HEIGHT = 1080;
constexpr const char *WINDOW_TITLE = "Prime Visualizer";

Application::Application()
    : renderer(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_TITLE) {
  if (state.divMode == 0)
    generator.GeneratePrimesInRange(FIRST_GEN);
  else
    generator.GenerateMultiplesInRange(state.divMode, FIRST_GEN);

  renderer.SyncGPUData(generator.GetPoints(), true);
}

void Application::Run() {
  while (!WindowShouldClose()) {
    float dt = GetFrameTime();

    size_t prevSize = generator.Size();
    inputManager.ProcessInputs(state, generator);

    // If input caused a reset like changing divisor
    if (generator.Size() < prevSize) {
      renderer.SyncGPUData(generator.GetPoints(), true);
    }

    Update(dt);

    // Sync any newly generated points to GPU VRAM
    renderer.SyncGPUData(generator.GetPoints(), false);

    renderer.BeginFrame();
    renderer.DrawScene(generator.GetPoints(), state);
    renderer.DrawUI(generator.GetPoints(), state);
    renderer.EndFrame();
  }
}

// Updates the limit of points on screen and calls for more if needed
/* void Application::Update(float dt) {
  state.currentLimit += state.primesPerSecond * dt;

  // Does not let the limit go negative to not render "negative amounts"
  if (state.currentLimit < 0.0f)
    state.currentLimit = 0.0f;

  if (state.currentLimit > generator.GetPoints().size()) {
    state.currentLimit = generator.GetPoints().size();
  }

  if (generator.GetPoints().size() - state.currentLimit <= THRESHOLD) {
    if (state.divMode == 0)
      generator.GeneratePrimesInRange(SEARCH_RANGE);
    else
      generator.GenerateMultiplesInRange(state.divMode, SEARCH_RANGE);
  }
}*/

// Passes the state and generator to Renderer so that it can alter them
// With the keyboard controls
// void Application::HandleInput() { renderer.ProcessInput(state, generator); }
void Application::Update(float dt) {
  state.currentLimit += state.primesPerSecond * dt;

  if (state.currentLimit < 0.0f)
    state.currentLimit = 0.0f;

  if (state.currentLimit > static_cast<float>(generator.Size())) {
    state.currentLimit = static_cast<float>(generator.Size());
  }

  // Dynamic runway threshold: trigger generation ~0.5s before we run out of
  // points
  size_t threshold =
      std::max(2000, static_cast<int>(state.primesPerSecond * 0.5f));

  if (generator.Size() - static_cast<size_t>(state.currentLimit) <= threshold) {

    // Target ~1.5 seconds of animation buffer (between 20k and 150k points per
    // batch)
    int targetPoints = std::clamp(
        static_cast<int>(state.primesPerSecond * 1.5f), 20000, 150000);

    if (state.divMode == 0) {
      // Prime Number Theorem: to get targetPoints, search range ≈ targetPoints
      // * ln(lastChecked)
      double x = static_cast<double>(
          std::max<uint64_t>(generator.GetLastChecked(), 10));
      int searchRange = static_cast<int>(targetPoints * std::log(x));
      searchRange = std::max(searchRange, 200000); // Minimum 200k search window

      generator.GeneratePrimesInRange(searchRange);
    } else {
      // For multiples of N: searchRange = targetPoints * N guarantees exactly
      // targetPoints numbers
      int searchRange = targetPoints * static_cast<int>(state.divMode);
      generator.GenerateMultiplesInRange(state.divMode, searchRange);
    }
  }
}
