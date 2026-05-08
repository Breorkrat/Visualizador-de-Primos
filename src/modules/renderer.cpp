module;
#include <raylib.h>
#include <raymath.h>
#include <rlgl.h>
#define RAYGUI_IMPLEMENTATION
#include <raygui.h>

#include <algorithm> // Para o std::lower_bound (sua nova busca binária)
#include <cmath>
#include <string>
#include <vector>
module renderer;

struct renderContext {
  int limitIdx, startIdx, step;
  float dotScale, invMaxP, rMax;
  float minX, maxX, minY, maxY;
  Color globalBreath;
};

// Raylib implementations for the PIMPL
struct Renderer::Impl {
  Camera2D camera;
  Texture2D dot;
  int width, height, centrox, centroy;
  char zoomMode = 0; // 0: Cover, 1: Full, 2: Free

  // Presets
  Color customStatic = RED;
  Color customGradientCenter = WHITE;
  Color customGradientEdge = BLACK;

  char inputBuffer[16] = {0};
  bool isTyping = false;

  const char *colorSchemeNames[4] = {"Calculated", "Breathing", "Custom Static",
                                     "Custom Gradient"};
  Color GetPointColor(const NumberPoint &pt, const GameState &state,
                      const renderContext &ctx) {
    float distRatio = static_cast<float>(pt.p) * ctx.invMaxP;

    switch (state.colorMode) {
    case ColorMode::Calculated:
      return ColorFromHSV(pt.hue, 0.8f, 1.0f);
    case ColorMode::Breathing:
      return ctx.globalBreath;
    case ColorMode::CustomStatic:
      return customStatic;
    case ColorMode::CustomGradient:
      return ColorLerp(customGradientCenter, customGradientEdge, distRatio);
    }
    return WHITE;
  }
  void DrawPointsMode(const std::vector<NumberPoint> &points,
                      const GameState &state, const renderContext &ctx) {
    // Only draws squares if up close
    bool isFarAway = (camera.zoom < 0.5f);

    if (isFarAway)
      rlBegin(RL_LINES);
    else
      rlBegin(RL_QUADS);

    // Draws each point, skipping by step
    for (int i = ctx.startIdx; i < ctx.limitIdx; i += ctx.step) {
      if (points[i].p > ctx.rMax)
        break;

      Vector2 pos = {(float)points[i].x, (float)points[i].y};
      if (pos.x < ctx.minX || pos.x > ctx.maxX || pos.y < ctx.minY ||
          pos.y > ctx.maxY)
        continue;

      // Gets the color of the current point
      Color drawColor = GetPointColor(points[i], state, ctx);
      rlColor4ub(drawColor.r, drawColor.g, drawColor.b, drawColor.a);

      if (isFarAway) {
        rlVertex2f(pos.x, pos.y);
        rlVertex2f(pos.x + ctx.dotScale, pos.y);
      } else {
        rlVertex2f(pos.x, pos.y);
        rlVertex2f(pos.x, pos.y + ctx.dotScale);
        rlVertex2f(pos.x + ctx.dotScale, pos.y + ctx.dotScale);
        rlVertex2f(pos.x + ctx.dotScale, pos.y);
      }
    }
    rlEnd();
  }

  void DrawWebMode(const std::vector<NumberPoint> &points,
                   const GameState &state, const renderContext &ctx) {
    rlBegin(RL_LINES);

    // Avoids starting "in the middle" between where 2 lines should be
    int safeStart = std::max(0, ctx.startIdx - ctx.step);

    for (int i = safeStart; i < ctx.limitIdx - ctx.step; i += ctx.step) {

      Vector2 p1 = {(float)points[i].x, (float)points[i].y};
      Vector2 p2 = {(float)points[i + ctx.step].x,
                    (float)points[i + ctx.step].y};

      // Gets the color for the line
      Color drawColor = GetPointColor(points[i], state, ctx);
      rlColor4ub(drawColor.r, drawColor.g, drawColor.b, drawColor.a);

      rlVertex2f(p1.x, p1.y);
      rlVertex2f(p2.x, p2.y);
    }
    rlEnd();
  }
};

Renderer::Renderer(int width, int height, const std::string &title) {
  impl = new Impl(); // Aloca as informações privadas
  impl->width = width;
  impl->height = height;
  impl->centrox = width / 2;
  impl->centroy = height / 2;

  SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_HIGHDPI | FLAG_WINDOW_RESIZABLE);
  InitWindow(width, height, title.c_str());
  SetTargetFPS(60);

  // Cria a textura do ponto
  Image img = GenImageColor(2, 2, WHITE);
  impl->dot = LoadTextureFromImage(img);
  UnloadImage(img);

  // Configura a câmera
  impl->camera = {0};
  impl->camera.target = {0.0f, 0.0f};
  impl->camera.offset = {(float)impl->centrox, (float)impl->centroy};
  impl->camera.rotation = 0.0f;
  impl->camera.zoom = 1.0f;
}

Renderer::~Renderer() {
  UnloadTexture(impl->dot);
  CloseWindow();
  delete impl; // Limpa a memória
}

bool Renderer::WindowShouldClose() const { return ::WindowShouldClose(); }
float Renderer::GetDeltaTime() const { return ::GetFrameTime(); }

void Renderer::BeginFrame() {
  if (IsWindowResized()) {
    impl->width = GetScreenWidth();
    impl->height = GetScreenHeight();
    impl->centrox = impl->width / 2;
    impl->centroy = impl->height / 2;
    impl->camera.offset = {(float)impl->centrox, (float)impl->centroy};
  }
  ::BeginDrawing();
  ClearBackground(BLACK);
}

void Renderer::EndFrame() { ::EndDrawing(); }

void Renderer::DrawScene(const std::vector<NumberPoint> &points,
                         const GameState &state) {
  if (points.empty() || state.currentLimit < 1.0f)
    return;

  // Updates camera based on gamestate
  impl->camera.target.x = state.camera.x;
  impl->camera.target.y = state.camera.y;
  impl->camera.zoom = state.camera.zoom;
  impl->zoomMode = state.camera.zoomMode;

  // Declares the variables useful for culling, color and render
  renderContext ctx;
  ctx.limitIdx = static_cast<int>(state.currentLimit);

  if (ctx.limitIdx > 0 && (impl->zoomMode == 0 || impl->zoomMode == 1)) {
    float maxRadius = static_cast<float>(points[ctx.limitIdx - 1].p);
    if (impl->zoomMode == 0)
      impl->camera.zoom = ((float)impl->width / 2.0f) / (maxRadius * 0.9f);
    else if (impl->zoomMode == 1)
      impl->camera.zoom = ((float)impl->height / 2.0f) / (maxRadius * 1.1f);
  }

  // Calculates bounding box of camera for culling
  Vector2 tl = GetScreenToWorld2D({0, 0}, impl->camera);
  Vector2 br = GetScreenToWorld2D({(float)impl->width, (float)impl->height},
                                  impl->camera);

  ctx.minX = std::min(tl.x, br.x);
  ctx.maxX = std::max(tl.x, br.x);
  ctx.minY = std::min(tl.y, br.y);
  ctx.maxY = std::max(tl.y, br.y);

  float cx = (ctx.minX > 0) ? ctx.minX : ((ctx.maxX < 0) ? ctx.maxX : 0);
  float cy = (ctx.minY > 0) ? ctx.minY : ((ctx.maxY < 0) ? ctx.maxY : 0);
  float rMin = std::sqrt(cx * cx + cy * cy);

  ctx.rMax =
      std::sqrt(std::max(tl.x * tl.x + tl.y * tl.y, br.x * br.x + br.y * br.y));

  // Searches the first element that can be on screen
  auto it = std::lower_bound(
      points.begin(), points.begin() + ctx.limitIdx, rMin,
      [](const NumberPoint &p, float val) { return p.p < val; });

  // Sets the other variables
  ctx.startIdx = std::distance(points.begin(), it);
  ctx.dotScale = std::max(2.0f, 1.0f / impl->camera.zoom);
  ctx.invMaxP = 1.0f / static_cast<float>(points[ctx.limitIdx - 1].p);
  ctx.globalBreath =
      ColorFromHSV(std::fmod(GetTime() * 50.0f, 360.0f), 0.7f, 1.0f);
  ctx.step = (impl->zoomMode == 2 && ctx.limitIdx > 100000) ? 5 : 1;

  // Starts 2D mode for drawing scene
  BeginMode2D(impl->camera);

  // Chooses how to draw, affects how culling is calculated
  if (state.drawAsWeb)
    impl->DrawWebMode(points, state, ctx);
  else
    impl->DrawPointsMode(points, state, ctx);

  // Stops 2D mode to draw UI later
  EndMode2D();
}

void Renderer::DrawUI(const std::vector<NumberPoint> &points,
                      const GameState &state) {
  if (state.showFPS)
    DrawFPS(impl->width - 100, 10);

  // Desenha a mira (Cursor) no centro
  if (state.showCursor) {
    constexpr int TAM_BAR_X = 10;
    constexpr int TAM_BAR_Y = 8;
    constexpr int BAR_THICKNESS = 1;
    DrawRectangle(impl->centrox - (TAM_BAR_X / 2), impl->centroy, TAM_BAR_X,
                  BAR_THICKNESS, WHITE);
    DrawRectangle(impl->centrox, impl->centroy - (TAM_BAR_Y / 2), BAR_THICKNESS,
                  TAM_BAR_Y, WHITE);
  }

  // Painéis de Seleção de Cor do RayGui
  if (state.colorPickerVisible == 1) {
    GuiColorPicker((Rectangle){(float)impl->width - 250, 50, 200, 200},
                   "Pick a color", &impl->customStatic);
  } else if (state.colorPickerVisible == 2) {
    GuiColorPicker((Rectangle){(float)impl->width - 250, 50, 200, 200},
                   "Center Color", &impl->customGradientCenter);
    GuiColorPicker((Rectangle){(float)impl->width - 250, 300, 200, 200},
                   "Edge Color", &impl->customGradientEdge);
  }

  // Painel de Estatísticas
  if (state.showStats) {
    unsigned long long primoAtual = 0;
    int limitInt = static_cast<int>(state.currentLimit);

    // Proteção para não acessar memória fora do vetor
    if (limitInt > 0 && limitInt <= points.size()) {
      primoAtual = points[limitInt - 1].p;
    }

    DrawText(TextFormat("Numbers rendered: %.0f", state.currentLimit), 10, 10,
             20, WHITE);
    DrawText(TextFormat("Current number: %llu", primoAtual), 10, 30, 20, WHITE);
    DrawText(TextFormat("PPS: %.2f", state.primesPerSecond), 10, 50, 20, WHITE);
    DrawText(TextFormat("Numbers calculated: %zu", points.size()), 10, 70, 20,
             WHITE);
    DrawText(
        TextFormat("Color scheme: %s",
                   impl->colorSchemeNames[static_cast<int>(state.colorMode)]),
        10, 90, 20, WHITE);

    if (state.divMode == 0) {
      DrawText("Rendering prime numbers", 10, 110, 20, WHITE);
    } else {
      DrawText(TextFormat("Rendering multiples of %d", state.divMode), 10, 110,
               20, WHITE);
    }
  }

  // Painel de Controles
  if (state.showControls) {
    DrawText("Scroll wheel to adjust the zoom", 10, impl->height - 30, 20,
             WHITE);
    DrawText("WASD to move the camera", 10, impl->height - 50, 20, WHITE);
    DrawText("R for camera mode Fill", 10, impl->height - 70, 20, WHITE);
    DrawText("F for camera mode (Full view)", 10, impl->height - 90, 20, WHITE);
    DrawText("Enter to pause", 10, impl->height - 110, 20, WHITE);
    DrawText("C to change color scheme", 10, impl->height - 130, 20, WHITE);
    DrawText("TAB to enter custom color modes", 10, impl->height - 150, 20,
             WHITE);
    DrawText("Arrow keys to change speed", 10, impl->height - 170, 20, WHITE);
    DrawText("P to take a screenshot", 10, impl->height - 190, 20, WHITE);
    DrawText("Left and right arrows to change number", 10, impl->height - 210,
             20, WHITE);
    DrawText("[F1-F4] to show/hide UI elements", 10, impl->height - 230, 20,
             WHITE);
    DrawText("B to enter Benchmark Mode", 10, impl->height - 250, 20, WHITE);
  }

  // Typing box
  if (impl->isTyping) {
    int boxX = impl->centrox - 150;
    int boxY = impl->centroy - 40;
    int boxWidth = 300;
    int boxHeight = 80;

    DrawRectangle(boxX, boxY, boxWidth, boxHeight, Fade(BLACK, 0.8f));
    DrawRectangleLines(boxX, boxY, boxWidth, boxHeight, RAYWHITE);
    DrawText("Jump to multiple of:", boxX + 20, boxY + 10, 20, GRAY);

    // Efeito de cursor piscando usando o tempo da Raylib
    const char *cursor = (std::fmod(GetTime(), 1.0) < 0.5) ? "_" : "";
    DrawText(TextFormat("%s%s", impl->inputBuffer, cursor), boxX + 20,
             boxY + 40, 30, WHITE);

    DrawText("[Enter] Confirm [Esc] Cancel", boxX + 20, boxY + 80, 10, GRAY);
  }
}
