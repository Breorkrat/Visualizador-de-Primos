module;
#include <raylib.h>
#include <raymath.h>
#include <rlgl.h>
#define RAYGUI_IMPLEMENTATION
#include <raygui.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string>
#include <vector>

module renderer;

// --- GLSL 330 Shader ---
static const char *COLOR_VS = R"(#version 330
layout(location = 0) in vec2 vertexPosition; // x, y

uniform mat4 mvp;
uniform float u_pointSize;
uniform float u_maxP;
uniform int u_colorMode;
uniform vec4 u_customStatic;
uniform vec4 u_gradientCenter;
uniform vec4 u_gradientEdge;
uniform vec4 u_globalBreath;

out vec4 fragColor;

vec3 hsv2rgb(vec3 c) {
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

void main() {
    gl_Position = mvp * vec4(vertexPosition.xy, 0.0, 1.0);
    gl_PointSize = u_pointSize;

    float pVal = length(vertexPosition);
    float distRatio = (u_maxP > 0.0) ? clamp(pVal / u_maxP, 0.0, 1.0) : 0.0;

    if (u_colorMode == 0) {
        // Calculated (HSV)
        float hue = mod(pVal * 0.05, 360.0) / 360.0;
        fragColor = vec4(hsv2rgb(vec3(hue, 0.8, 1.0)), 1.0);
    } else if (u_colorMode == 1) {
        // Breathing
        fragColor = u_globalBreath;
    } else if (u_colorMode == 2) {
        // Custom Static
        fragColor = u_customStatic;
    } else {
        // Custom Gradient
        fragColor = mix(u_gradientCenter, u_gradientEdge, distRatio);
    }
}
)";

static const char *COLOR_FS = R"(#version 330
in vec4 fragColor;
out vec4 finalColor;

void main() {
    finalColor = fragColor;
}
)";

struct Renderer::Impl {
  Camera2D camera;
  int width, height, centrox, centroy;
  char zoomMode = 0;

  // GPU Buffer State
  unsigned int vaoId = 0;
  unsigned int vboId = 0;
  size_t gpuAllocatedCapacity = 0;
  size_t gpuSyncedCount = 0;

  // Shader & Uniform Locations
  Shader colorShader;
  int locMvp;
  int locPointSize;
  int locMaxP;
  int locColorMode;
  int locCustomStatic;
  int locGradientCenter;
  int locGradientEdge;
  int locGlobalBreath;

  // Presets
  Color customStatic = RED;
  Color customGradientCenter = WHITE;
  Color customGradientEdge = BLACK;

  const char *colorSchemeNames[4] = {"Calculated", "Breathing", "Custom Static",
                                     "Custom Gradient"};

  void InitGPU() {
    colorShader = LoadShaderFromMemory(COLOR_VS, COLOR_FS);

    locMvp = GetShaderLocation(colorShader, "mvp");
    locPointSize = GetShaderLocation(colorShader, "u_pointSize");
    locMaxP = GetShaderLocation(colorShader, "u_maxP");
    locColorMode = GetShaderLocation(colorShader, "u_colorMode");
    locCustomStatic = GetShaderLocation(colorShader, "u_customStatic");
    locGradientCenter = GetShaderLocation(colorShader, "u_gradientCenter");
    locGradientEdge = GetShaderLocation(colorShader, "u_gradientEdge");
    locGlobalBreath = GetShaderLocation(colorShader, "u_globalBreath");

    // Initialize initial GPU buffer (capacity for 100k points, dynamically
    // grows)
    vaoId = rlLoadVertexArray();
    rlEnableVertexArray(vaoId);

    gpuAllocatedCapacity = 100000;
    vboId = rlLoadVertexBuffer(
        nullptr, gpuAllocatedCapacity * sizeof(NumberPoint), true);

    // Attribute 0: vec2 (x, y)
    rlSetVertexAttribute(0, 2, RL_FLOAT, false, sizeof(NumberPoint), 0);
    rlEnableVertexAttribute(0);

    rlDisableVertexArray();
  }

  void FreeGPU() {
    UnloadShader(colorShader);
    if (vboId > 0)
      rlUnloadVertexBuffer(vboId);
    if (vaoId > 0)
      rlUnloadVertexArray(vaoId);
  }

  void DrawPointLabels(const std::vector<NumberPoint> &points, int renderCount,
                       const GameState &state, float maxP, Color breathCol) {
    constexpr float TEXT_START_ZOOM = 1.2f;
    constexpr float TEXT_FULL_ZOOM = 2.2f;

    // Only show numbers when zoomed in close
    if (camera.zoom < TEXT_START_ZOOM)
      return;

    float alpha = std::clamp((camera.zoom - TEXT_START_ZOOM) /
                                 (TEXT_FULL_ZOOM - TEXT_START_ZOOM),
                             0.0f, 1.0f);
    int fontSize = std::clamp(static_cast<int>(7.0f * camera.zoom), 10, 160);
    int shadowOffset = std::max(1, fontSize / 16);

    // Smooth fade-in between zoom 2.0 and 4.0
    Color textColor = Fade(RAYWHITE, alpha);
    Color shadowColor = Fade(BLACK, alpha * 0.9f);

    // Viewport Bounding Box in World Coordinates
    Vector2 tl = GetScreenToWorld2D({0, 0}, camera);
    Vector2 br = GetScreenToWorld2D({(float)width, (float)height}, camera);

    float minX = std::min(tl.x, br.x);
    float maxX = std::max(tl.x, br.x);
    float minY = std::min(tl.y, br.y);
    float maxY = std::max(tl.y, br.y);

    float cx = (minX > 0) ? minX : ((maxX < 0) ? maxX : 0);
    float cy = (minY > 0) ? minY : ((maxY < 0) ? maxY : 0);
    float rMin = std::sqrt(cx * cx + cy * cy);
    float rMax = std::sqrt(
        std::max({tl.x * tl.x + tl.y * tl.y, br.x * br.x + br.y * br.y,
                  tl.x * tl.x + br.y * br.y, br.x * br.x + tl.y * tl.y}));

    // Binary search for first visible point
    auto it = std::lower_bound(
        points.begin(), points.begin() + renderCount, rMin,
        [](const NumberPoint &p, float val) { return (float)p.p < val; });

    size_t startIdx = std::distance(points.begin(), it);

    // Draw each number centered exactly on its point coordinates
    for (size_t i = startIdx; i < static_cast<size_t>(renderCount); ++i) {
      if (static_cast<float>(points[i].p) > rMax)
        break;

      if (points[i].x >= minX && points[i].x <= maxX && points[i].y >= minY &&
          points[i].y <= maxY) {

        Vector2 sPos = GetWorldToScreen2D({points[i].x, points[i].y}, camera);

        // Get color matching current scheme
        Color baseCol = RAYWHITE;
        if (state.colorMode == ColorMode::Calculated) {
          float hue =
              std::fmod(static_cast<float>(points[i].p) * 0.05f, 360.0f);
          baseCol = ColorFromHSV(hue, 0.8f, 1.0f);
        } else if (state.colorMode == ColorMode::Breathing) {
          baseCol = breathCol;
        } else if (state.colorMode == ColorMode::CustomStatic) {
          baseCol = customStatic;
        } else if (state.colorMode == ColorMode::CustomGradient) {
          float ratio = (maxP > 0.0f)
                            ? std::clamp(static_cast<float>(points[i].p) / maxP,
                                         0.0f, 1.0f)
                            : 0.0f;
          baseCol = ColorLerp(customGradientCenter, customGradientEdge, ratio);
        }

        Color textColor = Fade(baseCol, alpha);
        Color shadowColor = Fade(BLACK, alpha * 0.9f);

        const char *label = TextFormat("%u", points[i].p);
        int textWidth = MeasureText(label, fontSize);

        // Center text on (sPos.x, sPos.y)
        int drawX = (int)sPos.x - (textWidth / 2);
        int drawY = (int)sPos.y - (fontSize / 2);

        // Dark drop-shadow for contrast, followed by the centered number
        DrawText(label, drawX + shadowOffset, drawY + shadowOffset, fontSize,
                 shadowColor);
        DrawText(label, drawX, drawY, fontSize, textColor);
      }
    }
  }
};

Renderer::Renderer(int width, int height, const std::string &title) {
  impl = new Impl();
  impl->width = width;
  impl->height = height;
  impl->centrox = width / 2;
  impl->centroy = height / 2;

  SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_HIGHDPI | FLAG_WINDOW_RESIZABLE);
  InitWindow(width, height, title.c_str());
  SetTargetFPS(180);

  impl->camera.target = {0.0f, 0.0f};
  impl->camera.offset = {(float)impl->centrox, (float)impl->centroy};
  impl->camera.rotation = 0.0f;
  impl->camera.zoom = 1.0f;

  impl->InitGPU();
}

Renderer::~Renderer() {
  impl->FreeGPU();
  CloseWindow();
  delete impl;
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

void Renderer::SyncGPUData(const std::vector<NumberPoint> &points,
                           bool fullReset) {
  if (fullReset) {
    impl->gpuSyncedCount = 0;
  }

  size_t currentCount = points.size();
  if (currentCount == impl->gpuSyncedCount || points.empty())
    return;

  // If points exceed current GPU allocation, reallocate with 1.5x capacity
  if (currentCount > impl->gpuAllocatedCapacity) {
    size_t newCapacity =
        std::max(currentCount, impl->gpuAllocatedCapacity * 3 / 2);

    rlUnloadVertexBuffer(impl->vboId);
    rlUnloadVertexArray(impl->vaoId);

    impl->vaoId = rlLoadVertexArray();
    rlEnableVertexArray(impl->vaoId);

    impl->vboId = rlLoadVertexBuffer(
        nullptr, (int)(newCapacity * sizeof(NumberPoint)), true);
    rlSetVertexAttribute(0, 3, RL_FLOAT, false, sizeof(NumberPoint), 0);
    rlEnableVertexAttribute(0);
    rlDisableVertexArray();

    rlUpdateVertexBuffer(impl->vboId, points.data(),
                         (int)(currentCount * sizeof(NumberPoint)), 0);

    impl->gpuAllocatedCapacity = newCapacity;
    impl->gpuSyncedCount = currentCount;
  } else {
    // Incrementally upload only newly calculated points
    size_t newElements = currentCount - impl->gpuSyncedCount;
    size_t offsetBytes = impl->gpuSyncedCount * sizeof(NumberPoint);
    const void *dataPtr = points.data() + impl->gpuSyncedCount;

    rlUpdateVertexBuffer(impl->vboId, dataPtr,
                         (int)(newElements * sizeof(NumberPoint)),
                         (int)offsetBytes);
    impl->gpuSyncedCount = currentCount;
  }
}

void Renderer::DrawScene(const std::vector<NumberPoint> &points,
                         GameState &state) {
  int renderCount = static_cast<int>(state.currentLimit);
  if (renderCount <= 0 || points.empty())
    return;

  renderCount = std::min(renderCount, static_cast<int>(points.size()));

  impl->camera.target.x = state.camera.x;
  impl->camera.target.y = state.camera.y;
  impl->camera.zoom = state.camera.zoom;
  impl->zoomMode = state.camera.zoomMode;

  // Auto-Camera Zoom calculation
  if (impl->zoomMode == 0 || impl->zoomMode == 1) {
    float maxRadius = points[renderCount - 1].p;
    if (maxRadius > 0.0f) {
      if (impl->zoomMode == 0)
        impl->camera.zoom = ((float)impl->width / 2.0f) / (maxRadius * 0.9f);
      else if (impl->zoomMode == 1)
        impl->camera.zoom = ((float)impl->height / 2.0f) / (maxRadius * 1.1f);

      state.camera.zoom = impl->camera.zoom;
    }
  } else {
    impl->camera.zoom = state.camera.zoom;
  }
  // Flush Raylib's internal batch so ClearBackground() is drawn BEFORE our
  // GPU calls
  rlDrawRenderBatchActive();

  // --- Compute Camera MVP Matrix ---
  BeginMode2D(impl->camera);

  Matrix matModelView = rlGetMatrixModelview();
  Matrix matProjection = rlGetMatrixProjection();
  Matrix matMVP = MatrixMultiply(matModelView, matProjection);

  // Activate Shader & Upload Uniforms
  BeginShaderMode(impl->colorShader);

  constexpr float POINT_CUTOFF_ZOOM = 2.0f;

  // Send Uniforms to GPU
  float pointSize = (impl->camera.zoom >= POINT_CUTOFF_ZOOM) ? 0.0f : 2.5f;
  float maxP = points[renderCount - 1].p;
  int colorModeInt = static_cast<int>(state.colorMode);
  Vector4 cStatic = ColorNormalize(impl->customStatic);
  Vector4 cGradCenter = ColorNormalize(impl->customGradientCenter);
  Vector4 cGradEdge = ColorNormalize(impl->customGradientEdge);
  Color breathCol =
      ColorFromHSV(std::fmod(GetTime() * 50.0f, 360.0f), 0.7f, 1.0f);
  Vector4 cBreath = ColorNormalize(breathCol);

  SetShaderValueMatrix(impl->colorShader, impl->locMvp, matMVP);
  SetShaderValue(impl->colorShader, impl->locPointSize, &pointSize,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(impl->colorShader, impl->locMaxP, &maxP, SHADER_UNIFORM_FLOAT);
  SetShaderValue(impl->colorShader, impl->locColorMode, &colorModeInt,
                 SHADER_UNIFORM_INT);
  SetShaderValue(impl->colorShader, impl->locCustomStatic, &cStatic,
                 SHADER_UNIFORM_VEC4);
  SetShaderValue(impl->colorShader, impl->locGradientCenter, &cGradCenter,
                 SHADER_UNIFORM_VEC4);
  SetShaderValue(impl->colorShader, impl->locGradientEdge, &cGradEdge,
                 SHADER_UNIFORM_VEC4);
  SetShaderValue(impl->colorShader, impl->locGlobalBreath, &cBreath,
                 SHADER_UNIFORM_VEC4);

  // --- Single GPU Draw Call ---
  rlDisableBackfaceCulling();
  rlEnableVertexArray(impl->vaoId);

  if (state.drawAsWeb) {
    rlEnableWireMode();
    rlDrawVertexArray(0, renderCount);
    rlDisableWireMode();
  } else {
    rlEnablePointMode();
    rlDrawVertexArray(0, renderCount);
    rlDisableWireMode();
  }

  rlDisableVertexArray();
  EndShaderMode();
  EndMode2D();

  impl->DrawPointLabels(points, renderCount, state, maxP, breathCol);
}

void Renderer::DrawUI(const std::vector<NumberPoint> &points,
                      const GameState &state) {
  if (state.showFPS)
    DrawFPS(impl->width - 100, 10);

  if (state.showCursor) {
    constexpr int TAM_BAR_X = 10;
    constexpr int TAM_BAR_Y = 8;
    constexpr int BAR_THICKNESS = 1;
    DrawRectangle(impl->centrox - (TAM_BAR_X / 2), impl->centroy, TAM_BAR_X,
                  BAR_THICKNESS, WHITE);
    DrawRectangle(impl->centrox, impl->centroy - (TAM_BAR_Y / 2), BAR_THICKNESS,
                  TAM_BAR_Y, WHITE);
  }

  if (state.colorPickerVisible == 1) {
    GuiColorPicker((Rectangle){(float)impl->width - 250, 50, 200, 200},
                   "Pick a color", &impl->customStatic);
  } else if (state.colorPickerVisible == 2) {
    GuiColorPicker((Rectangle){(float)impl->width - 250, 50, 200, 200},
                   "Center Color", &impl->customGradientCenter);
    GuiColorPicker((Rectangle){(float)impl->width - 250, 300, 200, 200},
                   "Edge Color", &impl->customGradientEdge);
  }

  if (state.showStats) {
    uint64_t primoAtual = 0;
    int limitInt = static_cast<int>(state.currentLimit);

    if (limitInt > 0 && limitInt <= static_cast<int>(points.size())) {
      primoAtual = static_cast<uint64_t>(points[limitInt - 1].p);
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

  if (state.isTyping) {
    int boxX = impl->centrox - 150;
    int boxY = impl->centroy - 40;
    int boxWidth = 300;
    int boxHeight = 80;

    DrawRectangle(boxX, boxY, boxWidth, boxHeight, Fade(BLACK, 0.8f));
    DrawRectangleLines(boxX, boxY, boxWidth, boxHeight, RAYWHITE);
    DrawText("Jump to multiple of:", boxX + 20, boxY + 10, 20, GRAY);

    const char *cursor = (std::fmod(GetTime(), 1.0) < 0.5) ? "_" : "";
    DrawText(TextFormat("%s%s", state.inputBuffer, cursor), boxX + 20,
             boxY + 40, 30, WHITE);
    DrawText("[Enter] Confirm [Esc] Cancel", boxX + 20, boxY + 80, 10, GRAY);
  }
}
