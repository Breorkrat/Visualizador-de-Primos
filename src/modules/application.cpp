module;
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
}

void Application::Run() {
  while (!WindowShouldClose()) {
    float dt = GetFrameTime();

    inputManager.ProcessInputs(state, generator);
    Update(dt);

    renderer.BeginFrame();
    renderer.DrawScene(generator.GetPoints(), state);
    renderer.DrawUI(generator.GetPoints(), state);
    renderer.EndFrame();
  }
}

// Updates the limit of points on screen and calls for more if needed
void Application::Update(float dt) {
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
}

// Passes the state and generator to Renderer so that it can alter them
// With the keyboard controls
// void Application::HandleInput() { renderer.ProcessInput(state, generator); }
