module;
#include <raylib.h>
#include <raymath.h>
#define RAYGUI_IMPLEMENTATION
#include <raygui.h>

#include <algorithm> // Para o std::lower_bound (sua nova busca binária)
#include <cmath>
#include <string>
#include <vector>
module renderer;

// AQUI DENTRO MORAM AS VARIÁVEIS DA RAYLIB!
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

void Renderer::ProcessInput(GameState &state, NumberGenerator &generator) {
  // If not paused, pps increases with the camera zoomout
  if (!state.ppsLock) {
    state.primesPerSecond =
        500.0f + (50.0f / std::sqrt(impl->camera.zoom)) * 1.0f;
    if (state.primesPerSecond > 30000.0f)
      state.primesPerSecond = 30000.0f;
  }
  float moveSpeed = 10.0f / impl->camera.zoom;

  // Processes input
  // Continuously altering keys
  if (IsKeyDown(KEY_W))
    impl->camera.target.y -= moveSpeed;
  if (IsKeyDown(KEY_S))
    impl->camera.target.y += moveSpeed;
  if (IsKeyDown(KEY_A))
    impl->camera.target.x -= moveSpeed;
  if (IsKeyDown(KEY_D))
    impl->camera.target.x += moveSpeed;

  float mouseWheel = GetMouseWheelMove();
  if (mouseWheel != 0) {
    impl->zoomMode = 2;
    impl->camera.zoom += (mouseWheel * 0.1f * impl->camera.zoom);
  }
  // Controlls PointsPerSecond
  if (IsKeyDown(KEY_UP)) {
    state.ppsLock = true;
    state.primesPerSecond += 10.0f;
  }
  if (IsKeyDown(KEY_DOWN)) {
    state.ppsLock = true;
    state.primesPerSecond -= 10.0f;
  }

  // Typing number implementation
  int character = GetCharPressed();
  if (character >= '0' && character <= '9') {
    impl->isTyping = true;
    int len = std::char_traits<char>::length(impl->inputBuffer);
    if (len < 15) {
      impl->inputBuffer[len] = (char)character;
      impl->inputBuffer[len + 1] = '\0';
    }
  }

  if (impl->isTyping) {
    if (IsKeyPressed(KEY_BACKSPACE)) {
      int len = std::char_traits<char>::length(impl->inputBuffer);
      if (len > 0)
        impl->inputBuffer[len - 1] = '\0';
      else
        impl->isTyping = false;
    }
    if (IsKeyPressed(KEY_ENTER)) {
      state.divMode = std::atoi(impl->inputBuffer);
      generator.Reset();
      state.currentLimit = 0;
      impl->inputBuffer[0] = '\0';
      impl->isTyping = false;
    }
    if (IsKeyPressed(KEY_ESCAPE)) {
      impl->inputBuffer[0] = '\0';
      impl->isTyping = false;
    }
  }

  // Keys processed once on keypress, not repeatedly
  int key = GetKeyPressed();
  while (key > 0) {
    if (key == KEY_F1)
      state.showControls = !state.showControls;
    if (key == KEY_F2)
      state.showFPS = !state.showFPS;
    if (key == KEY_F3)
      state.showStats = !state.showStats;

    if (key == KEY_R) {
      impl->zoomMode = 0;
      impl->camera.target = {0.0f, 0.0f};
    }
    if (key == KEY_F) {
      impl->zoomMode = 1;
      impl->camera.target = {0.0f, 0.0f};
    }

    // Custom colors
    if (key == KEY_TAB) {
      // Alters between 0 (Hidden), 1 (Solid) and 2 (Gradient)
      state.colorPickerVisible = (state.colorPickerVisible + 1) % 3;
    }

    // Pause/unpause rendering. Only when not typing
    if (key == KEY_ENTER && !impl->isTyping) {
      if (!state.ppsLock) {
        state.ppsLock = true;
        state.primesPerSecond = 0.0f;
      } else {
        state.ppsLock = false;
      }
    }

    // Screenshot
    if (key == KEY_P) {
      TakeScreenshot(TextFormat("prime_spiral_%d.png",
                                static_cast<int>(state.currentLimit)));
    }

    // Change color mode
    if (key == KEY_C)
      state.colorMode =
          static_cast<ColorMode>((static_cast<int>(state.colorMode) + 1) % 4);

    if (key == KEY_RIGHT) {
      state.divMode++;
      generator.Reset();
      state.currentLimit = 0;
    }
    if (key == KEY_LEFT && state.divMode > 0) {
      state.divMode--;
      generator.Reset();
      state.currentLimit = 0;
    }
    // Benchmark
    if (IsKeyPressed(KEY_B)) {
      state.benchmarkMode = !state.benchmarkMode;

      if (state.benchmarkMode) {
        generator.Reset(); // Limpa tudo
        // 50 milhões de espaço de busca gera aproximadamente 3 milhões de
        // primos
        generator.GeneratePrimesInRange(50000000);

        // Joga todos de uma vez só na tela (ignora a animação)
        state.currentLimit = generator.GetPoints().size();
        state.showFPS = true; // Força o FPS na tela

        // Bota a câmera num zoom ideal para ver o borrão inteiro
        impl->camera.target = {0.0f, 0.0f};
        impl->camera.zoom = 0.05f;
      } else {
        generator.Reset();
        state.currentLimit = 0;
        impl->camera.zoom = 1.0f;
      }
    }
    key = GetKeyPressed();
  }
}

void Renderer::DrawPoints(const std::vector<NumberPoint> &points,
                          const GameState &state) {
  if (points.empty() || state.currentLimit < 1.0f)
    return;

  BeginMode2D(impl->camera);

  // Calculates the screen limits for culling (top left, bottom right, etc.)
  Vector2 tl = GetScreenToWorld2D({0, 0}, impl->camera);
  Vector2 br = GetScreenToWorld2D({(float)impl->width, (float)impl->height},
                                  impl->camera);

  // Finds the coordinates closest to the center
  float minX = std::min(tl.x, br.x);
  float maxX = std::max(tl.x, br.x);
  float minY = std::min(tl.y, br.y);
  float maxY = std::max(tl.y, br.y);

  // Calculates the camera point closest to ceter
  float cx = (minX > 0) ? minX : ((maxX < 0) ? maxX : 0);
  float cy = (minY > 0) ? minY : ((maxY < 0) ? maxY : 0);
  float rMin = std::sqrt(cx * cx + cy * cy);
  float rMax =
      std::sqrt(std::max(tl.x * tl.x + tl.y * tl.y, br.x * br.x + br.y * br.y));

  // Binary searches the first possible element on screen
  auto it = std::lower_bound(
      points.begin(), points.begin() + static_cast<int>(state.currentLimit),
      rMin, [](const NumberPoint &p, float val) { return p.p < val; });

  int startIdx = std::distance(points.begin(), it);
  int limitIdx = static_cast<int>(state.currentLimit);
  if (limitIdx > 0 && (impl->zoomMode == 0 || impl->zoomMode == 1)) {
    float maxRadius = static_cast<float>(points[limitIdx - 1].p);
    if (impl->zoomMode == 0) {
      impl->camera.zoom = ((float)impl->width / 2.0f) / (maxRadius * 0.9f);
    } else if (impl->zoomMode == 1) {
      impl->camera.zoom = ((float)impl->height / 2.0f) / (maxRadius * 1.1f);
    }
  }

  float dotScale = std::max(2.0f, 1.0f / impl->camera.zoom);
  float invMaxP = 1.0f / static_cast<float>(points[limitIdx - 1].p);
  Color globalBreath =
      ColorFromHSV(std::fmod(GetTime() * 50.0f, 360.0f), 0.7f, 1.0f);

  // LOD step if moving and more than 100K elements on screen
  bool isMoving = IsKeyDown(KEY_W) || IsKeyDown(KEY_A) || IsKeyDown(KEY_S) ||
                  IsKeyDown(KEY_D) || GetMouseWheelMove() != 0;
  int step = (isMoving && limitIdx > 100000) ? 5 : 1;

  for (int i = startIdx; i < limitIdx; i += step) {
    if (points[i].p > rMax)
      break;

    Vector2 pos = {(float)points[i].x, (float)points[i].y};
    if (pos.x < minX || pos.x > maxX || pos.y < minY || pos.y > maxY)
      continue;

    Color drawColor = WHITE;
    float distRatio = static_cast<float>(points[i].p) * invMaxP;

    switch (state.colorMode) {
    case ColorMode::Calculated:
      drawColor = ColorFromHSV(points[i].hue, 0.8f, 1.0f);
      break;
    case ColorMode::Breathing:
      drawColor = globalBreath;
      break;
    case ColorMode::CustomStatic:
      drawColor = impl->customStatic;
      break;
    case ColorMode::CustomGradient:
      drawColor = ColorLerp(impl->customGradientCenter,
                            impl->customGradientEdge, distRatio);
      break;
    }

    DrawTextureEx(impl->dot, pos, 0.0f, dotScale, drawColor);
  }

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
    DrawText("B to enter Benchmark Mode", 10, impl->height - 250, 20, YELLOW);
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
