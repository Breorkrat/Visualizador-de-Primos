module;

#define RAYGUI_IMPLEMENTATION

#include <GL/gl.h>
#include <raygui.h>
#include <raylib.h>
#include <raymath.h>
#include <rlgl.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string>
#include <vector>

module renderer;

import shaders;

struct Renderer::Impl {
  Camera2D camera;
  int width, height, centrox, centroy;
  char zoomMode = 0;

  // GPU Buffer State
  unsigned int vaoId = 0;
  unsigned int vboId = 0;
  unsigned int ngonEboId = 0;
  size_t gpuAllocatedCapacity = 0;
  size_t gpuSyncedCount = 0;
  int currentEboN = 0;

  // Shader & Uniform Locations
  Shader colorShader;
  int locMvp;
  int locTime;
  int locPointSize;
  int locMaxP;
  int locColorMode;
  int locCustomStatic;
  int locGradientCenter;
  int locGradientEdge;
  int locGlobalBreath;

  int locRippleEnabled;
  int locRippleSpeed;
  int locRippleIntensity;

  // Presets
  Color customStatic = RED;
  Color customGradientCenter = WHITE;
  Color customGradientEdge = BLACK;

  const char *colorSchemeNames[4] = {"Calculated", "Breathing", "Custom Static",
                                     "Custom Gradient"};

  // Helper for closing shapes of size n
  void GenerateNgonIndices(size_t maxVertices, int N) {
    if (N < 3)
      return;
    if (ngonEboId > 0) {
      rlUnloadVertexBuffer(ngonEboId);
    }

    size_t numShapes = maxVertices / N;
    std::vector<unsigned int> indices;
    indices.reserve(numShapes * 2 * N);

    for (unsigned int i = 0; i + (N - 1) < maxVertices; i += N) {
      // Connect vertices: i -> i+1 -> i+2 ... -> i+(N-1)
      for (int k = 0; k < N - 1; ++k) {
        indices.push_back(i + k);
        indices.push_back(i + k + 1);
      }
      // Close the loop: i+(N-1) -> i
      indices.push_back(i + (N - 1));
      indices.push_back(i);
    }

    ngonEboId = rlLoadVertexBufferElement(
        indices.data(), (int)(indices.size() * sizeof(unsigned int)), false);
    currentEboN = N;
  }

  void InitGPU() {
    colorShader = LoadShaderFromMemory(SPIRAL_VS, SPIRAL_FS);

    locMvp = GetShaderLocation(colorShader, "mvp");
    locTime = GetShaderLocation(colorShader, "u_time");
    locPointSize = GetShaderLocation(colorShader, "u_pointSize");
    locMaxP = GetShaderLocation(colorShader, "u_maxP");
    locColorMode = GetShaderLocation(colorShader, "u_colorMode");
    locCustomStatic = GetShaderLocation(colorShader, "u_customStatic");
    locGradientCenter = GetShaderLocation(colorShader, "u_gradientCenter");
    locGradientEdge = GetShaderLocation(colorShader, "u_gradientEdge");
    locGlobalBreath = GetShaderLocation(colorShader, "u_globalBreath");

    locRippleEnabled = GetShaderLocation(colorShader, "u_rippleEnabled");
    locRippleSpeed = GetShaderLocation(colorShader, "u_rippleSpeed");
    locRippleIntensity = GetShaderLocation(colorShader, "u_rippleIntensity");

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

    GenerateNgonIndices(gpuAllocatedCapacity, 3);
  }

  void FreeGPU() {
    UnloadShader(colorShader);
    if (vboId > 0)
      rlUnloadVertexBuffer(vboId);
    if (ngonEboId > 0)
      rlUnloadVertexBuffer(ngonEboId);
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
  void DrawSideMenu(GameState &state) {
    if (!state.showSideMenu) {
      // Floating button in top-right corner when closed
      if (GuiButton({(float)width - 110, 10, 100, 30}, "#141# Settings")) {
        state.showSideMenu = true;
      }
      return;
    }

    float menuWidth = 340.0f;
    float menuX = (float)width - menuWidth - 10.0f;
    float menuY = 10.0f;
    float menuHeight = (float)height - 20.0f;

    // Dark semi-transparent background panel
    DrawRectangle((int)menuX, (int)menuY, (int)menuWidth, (int)menuHeight,
                  Fade(BLACK, 0.88f));
    DrawRectangleLines((int)menuX, (int)menuY, (int)menuWidth, (int)menuHeight,
                       DARKGRAY);

    // Title Bar & Close Button
    DrawText("SETTINGS [TAB to Close]", (int)menuX + 15, (int)menuY + 12, 18,
             RAYWHITE);
    if (GuiButton({menuX + menuWidth - 35, menuY + 8, 25, 25}, "X")) {
      state.showSideMenu = false;
    }

    // Category Tabs Bar
    float tabY = menuY + 45;
    if (GuiButton({menuX + 10, tabY, 70, 26}, "Visuals"))
      state.activeMenuTab = MenuTab::Visuals;
    if (GuiButton({menuX + 85, tabY, 70, 26}, "Colors"))
      state.activeMenuTab = MenuTab::Colors;
    if (GuiButton({menuX + 160, tabY, 80, 26}, "Sim"))
      state.activeMenuTab = MenuTab::Simulation;
    if (GuiButton({menuX + 245, tabY, 85, 26}, "HUD"))
      state.activeMenuTab = MenuTab::HUD;

    DrawLine((int)menuX + 10, (int)tabY + 32, (int)menuX + (int)menuWidth - 10,
             (int)tabY + 32, GRAY);

    float cy = tabY + 45;

    // --- Active Tab Content Dispatch ---
    switch (state.activeMenuTab) {
    case MenuTab::Visuals: {
      GuiCheckBox({menuX + 15, cy, 20, 20}, "Enable Ripple Wave",
                  &state.visuals.rippleEnabled);
      cy += 35;

      GuiLabel({menuX + 15, cy, 200, 20},
               TextFormat("Ripple Speed: %.1f", state.visuals.rippleSpeed));
      cy += 20;
      GuiSliderBar({menuX + 15, cy, 220, 18}, "", "",
                   &state.visuals.rippleSpeed, 0.0f, 15.0f);
      cy += 30;

      GuiLabel(
          {menuX + 15, cy, 200, 20},
          TextFormat("Ripple Intensity: %.3f", state.visuals.rippleIntensity));
      cy += 20;
      GuiSliderBar({menuX + 15, cy, 220, 18}, "", "",
                   &state.visuals.rippleIntensity, 0.0f, 0.3f);
      cy += 35;

      GuiLabel({menuX + 15, cy, 200, 20},
               TextFormat("Breathing Speed: %.1f", state.visuals.breathSpeed));
      cy += 20;
      GuiSliderBar({menuX + 15, cy, 220, 18}, "", "",
                   &state.visuals.breathSpeed, 5.0f, 150.0f);
      cy += 35;

      GuiLabel({menuX + 15, cy, 200, 20},
               TextFormat("Base Point Size: %.1f px", state.visuals.pointSize));
      cy += 20;
      GuiSliderBar({menuX + 15, cy, 220, 18}, "", "", &state.visuals.pointSize,
                   1.0f, 8.0f);
      break;
    }

    case MenuTab::Colors: {
      DrawText("Color Mode:", (int)menuX + 15, (int)cy, 16, RAYWHITE);
      cy += 25;

      if (GuiButton({menuX + 15, cy, 145, 26}, "Calculated (HSV)"))
        state.colorMode = ColorMode::Calculated;
      if (GuiButton({menuX + 165, cy, 145, 26}, "Breathing"))
        state.colorMode = ColorMode::Breathing;
      cy += 32;
      if (GuiButton({menuX + 15, cy, 145, 26}, "Custom Static"))
        state.colorMode = ColorMode::CustomStatic;
      if (GuiButton({menuX + 165, cy, 145, 26}, "Custom Gradient"))
        state.colorMode = ColorMode::CustomGradient;
      cy += 40;

      switch (state.colorMode) {
      case ColorMode::CustomStatic:
        DrawText("Static Color Picker:", (int)menuX + 15, (int)cy, 14, GRAY);
        cy += 20;
        GuiColorPicker({menuX + 40, cy, 180, 180}, "Pick Color", &customStatic);
        break;

      case ColorMode::CustomGradient:
        DrawText("Center Color:", (int)menuX + 15, (int)cy, 14, GRAY);
        cy += 18;
        GuiColorPicker({menuX + 40, cy, 120, 120}, "Center",
                       &customGradientCenter);
        cy += 135;
        DrawText("Edge Color:", (int)menuX + 15, (int)cy, 14, GRAY);
        cy += 18;
        GuiColorPicker({menuX + 40, cy, 120, 120}, "Edge", &customGradientEdge);
        break;

      default:
        break;
      }
      break;
    }

    case MenuTab::Simulation: {
      GuiLabel({menuX + 15, cy, 200, 20},
               TextFormat("Primes/Sec: %.0f", state.primesPerSecond));
      cy += 20;

      // Only lock PPS when the user actually drags the slider
      if (GuiSliderBar({menuX + 15, cy, 220, 18}, "", "",
                       &state.primesPerSecond, 0.0f, 10000.0f)) {
        state.ppsLock = true;
      }
      cy += 35;

      GuiCheckBox({menuX + 15, cy, 20, 20}, "Lock PPS (Manual Speed)",
                  &state.ppsLock);
      cy += 35;

      DrawText(TextFormat("Current Divisor: %u", state.divMode),
               (int)menuX + 15, (int)cy, 16, RAYWHITE);
      cy += 25;
      break;
    }

    case MenuTab::HUD: {
      GuiCheckBox({menuX + 15, cy, 20, 20}, "Show FPS", &state.showFPS);
      cy += 30;
      GuiCheckBox({menuX + 15, cy, 20, 20}, "Show Statistics Panel",
                  &state.showStats);
      cy += 30;
      GuiCheckBox({menuX + 15, cy, 20, 20}, "Show Center Crosshair",
                  &state.showCursor);
      cy += 30;
      GuiCheckBox({menuX + 15, cy, 20, 20}, "Render Web (Lines)",
                  &state.drawAsWeb);
      cy += 30;

      if (state.drawAsWeb) {
        const char *shapeName = "Custom N-gon";
        if (state.visuals.webShapeSides <= 0)
          shapeName = "Continuous Line (0)";
        else if (state.visuals.webShapeSides == 2)
          shapeName = "Disjoint Pairs (2)";
        else if (state.visuals.webShapeSides == 3)
          shapeName = "Triangles (3)";
        else if (state.visuals.webShapeSides == 4)
          shapeName = "Quads (4)";
        else if (state.visuals.webShapeSides == 5)
          shapeName = "Pentagons (5)";
        else if (state.visuals.webShapeSides == 6)
          shapeName = "Hexagons (6)";
        else if (state.visuals.webShapeSides == 8)
          shapeName = "Octagons (8)";

        DrawText(TextFormat("Shape: %s", shapeName), (int)menuX + 15, (int)cy,
                 14, RAYWHITE);
        cy += 20;

        // Quick Preset Buttons
        if (GuiButton({menuX + 15, cy, 65, 24}, "Line (0)"))
          state.visuals.webShapeSides = 0;
        if (GuiButton({menuX + 85, cy, 65, 24}, "Pairs (2)"))
          state.visuals.webShapeSides = 2;
        if (GuiButton({menuX + 155, cy, 65, 24}, "Tris (3)"))
          state.visuals.webShapeSides = 3;
        if (GuiButton({menuX + 225, cy, 65, 24}, "Quads (4)"))
          state.visuals.webShapeSides = 4;
        cy += 30;

        // Interactive N Slider (only updates when dragged)
        float nFloat = (float)std::max(3, state.visuals.webShapeSides);
        float prevN = nFloat;

        GuiLabel({menuX + 15, cy, 200, 20},
                 TextFormat("Polygon Sides (N): %d",
                            (state.visuals.webShapeSides < 3)
                                ? 3
                                : state.visuals.webShapeSides));
        cy += 20;

        GuiSliderBar({menuX + 15, cy, 220, 18}, "", "", &nFloat, 3.0f, 16.0f);

        // Only update if the user interacted with the slider
        if (std::abs(nFloat - prevN) > 0.001f) {
          state.visuals.webShapeSides = (int)std::round(nFloat);
        }
        cy += 35;
      }

      DrawText("Camera Presets:", (int)menuX + 15, (int)cy, 16, RAYWHITE);
      cy += 25;
      if (GuiButton({menuX + 15, cy, 120, 28}, "Fit Cover (R)")) {
        state.camera.zoomMode = 0;
        state.camera.x = 0.0f;
        state.camera.y = 0.0f;
      }
      if (GuiButton({menuX + 145, cy, 120, 28}, "Fit Full (F)")) {
        state.camera.zoomMode = 1;
        state.camera.x = 0.0f;
        state.camera.y = 0.0f;
      }
      break;
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

    if (impl->currentEboN >= 3) {
      impl->GenerateNgonIndices(newCapacity, impl->currentEboN);
    }
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

  float currentTime = static_cast<float>(GetTime());
  int rippleEnabledInt = state.visuals.rippleEnabled ? 1 : 0;
  SetShaderValue(impl->colorShader, impl->locTime, &currentTime,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(impl->colorShader, impl->locRippleEnabled, &rippleEnabledInt,
                 SHADER_UNIFORM_INT);
  SetShaderValue(impl->colorShader, impl->locRippleSpeed,
                 &state.visuals.rippleSpeed, SHADER_UNIFORM_FLOAT);
  SetShaderValue(impl->colorShader, impl->locRippleIntensity,
                 &state.visuals.rippleIntensity, SHADER_UNIFORM_FLOAT);

  // --- Single GPU Draw Call ---
  rlDisableBackfaceCulling();
  rlEnableVertexArray(impl->vaoId);

  if (state.drawAsWeb) {
    int N = state.visuals.webShapeSides;
    if (N <= 0) {
      glDrawArrays(GL_LINE_STRIP, 0, renderCount);
    } else if (N == 2) {
      glDrawArrays(GL_LINES, 0, renderCount);
    } else {
      if (N != impl->currentEboN) {
        impl->GenerateNgonIndices(impl->gpuAllocatedCapacity, N);
      }

      int numShapes = renderCount / N;
      int indexCount = numShapes * 2 * N;

      if (indexCount > 0) {
        rlEnableVertexBufferElement(impl->ngonEboId);
        glDrawElements(GL_LINES, indexCount, GL_UNSIGNED_INT, 0);
        rlDisableVertexBufferElement();
      }
    }
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
                      GameState &state) {
  impl->DrawSideMenu(state);

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
