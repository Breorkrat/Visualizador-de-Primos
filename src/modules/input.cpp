module;
#include <cmath>
#include <raylib.h>
#include <string>
module input;

void InputManager::ProcessInputs(GameState &state, NumberGenerator &generator) {
  HandleContinuous(state);
  HandlePressed(state, generator);
}

void InputManager::HandleContinuous(GameState &state) {
  // Dynamic Auto PPS based on current zoom
  if (!state.ppsLock) {
    if (state.camera.zoom > 0.0f) {
      state.primesPerSecond = 500.0f + (50.0f / std::sqrt(state.camera.zoom));
    }
    if (state.primesPerSecond > 200000.0f)
      state.primesPerSecond = 200000.0f;
  }

  // Camera Movement
  float moveSpeed = 15.0f / state.camera.zoom;
  if (IsKeyDown(KEY_W))
    state.camera.y -= moveSpeed;
  if (IsKeyDown(KEY_S))
    state.camera.y += moveSpeed;
  if (IsKeyDown(KEY_A))
    state.camera.x -= moveSpeed;
  if (IsKeyDown(KEY_D))
    state.camera.x += moveSpeed;

  // Manual Speed
  if (IsKeyDown(KEY_UP)) {
    state.ppsLock = true;
    state.primesPerSecond += 50.0f;
  }
  if (IsKeyDown(KEY_DOWN)) {
    state.ppsLock = true;
    state.primesPerSecond = std::max(0.0f, state.primesPerSecond - 50.0f);
  }

  // Smooth Zoom Centered on Mouse Cursor
  float mouseWheel = GetMouseWheelMove();
  if (mouseWheel != 0.0f) {
    Camera2D tempCam = {0};
    tempCam.target = {state.camera.x, state.camera.y};
    tempCam.offset = {GetScreenWidth() / 2.0f, GetScreenHeight() / 2.0f};
    tempCam.zoom = state.camera.zoom;

    Vector2 mouseWorldBefore = GetScreenToWorld2D(GetMousePosition(), tempCam);

    state.camera.zoomMode = 2; // Switch to Free Mode

    // Zoom multiplier
    float scaleFactor = 1.0f + (0.25f * std::abs(mouseWheel));
    if (mouseWheel < 0.0f)
      scaleFactor = 1.0f / scaleFactor;

    state.camera.zoom *= scaleFactor;
    tempCam.zoom = state.camera.zoom;

    Vector2 mouseWorldAfter = GetScreenToWorld2D(GetMousePosition(), tempCam);

    // Adjust target so point under mouse stays stationary
    state.camera.x += (mouseWorldBefore.x - mouseWorldAfter.x);
    state.camera.y += (mouseWorldBefore.y - mouseWorldAfter.y);
  }
}

void InputManager::HandlePressed(GameState &state, NumberGenerator &generator) {
  int key = GetKeyPressed();
  while (key > 0) {
    if (state.isTyping) {
      HandleTyping(state, generator, key);
    } else {
      if (key == KEY_F1)
        state.showControls = !state.showControls;

      if (key == KEY_F2)
        state.showFPS = !state.showFPS;

      if (key == KEY_F3)
        state.showStats = !state.showStats;

      if (key == KEY_F12)
        state.drawAsWeb = !state.drawAsWeb;

      // F and R reset camera back to auto-fit view
      if (key == KEY_R) {
        state.camera.zoomMode = 0;
        state.camera.x = 0.0f;
        state.camera.y = 0.0f;
      }
      if (key == KEY_F) {
        state.camera.zoomMode = 1;
        state.camera.x = 0.0f;
        state.camera.y = 0.0f;
      }

      if (key == KEY_C)
        state.colorMode =
            static_cast<ColorMode>((static_cast<int>(state.colorMode) + 1) % 4);

      if (key == KEY_TAB) {
        state.colorPickerVisible = (state.colorPickerVisible + 1) % 3;
      }

      if (key == KEY_ENTER && !state.isTyping) {
        if (!state.ppsLock) {
          state.ppsLock = true;
          state.primesPerSecond = 0.0f;
        } else {
          state.ppsLock = false;
        }
      }

      if (key == KEY_P) {
        TakeScreenshot(TextFormat("prime_spiral_%d.png",
                                  static_cast<int>(state.currentLimit)));
      }

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

      if (IsKeyPressed(KEY_B)) {
        state.benchmarkMode = !state.benchmarkMode;

        if (state.benchmarkMode) {
          generator.Reset();
          generator.GeneratePrimesInRange(50000000);
          state.currentLimit = static_cast<float>(generator.Size());
          state.showFPS = true;
          state.camera.x = 0.0f;
          state.camera.y = 0.0f;
          state.camera.zoomMode = 1;
        } else {
          generator.Reset();
          state.currentLimit = 0;
          state.camera.zoom = 1.0f;
          state.camera.zoomMode = 0;
        }
      }
    }
    key = GetKeyPressed();
  }

  int character = GetCharPressed();
  if (character >= '0' && character <= '9') {
    state.isTyping = true;
    int len = std::char_traits<char>::length(state.inputBuffer);
    if (len < 15) {
      state.inputBuffer[len] = (char)character;
      state.inputBuffer[len + 1] = '\0';
    }
  }
}

void InputManager::HandleTyping(GameState &state, NumberGenerator &generator,
                                int key) {
  if (key == KEY_BACKSPACE) {
    int len = std::char_traits<char>::length(state.inputBuffer);
    if (len > 0)
      state.inputBuffer[len - 1] = '\0';
    else
      state.isTyping = false;
  }
  if (key == KEY_ENTER) {
    state.divMode = std::atoi(state.inputBuffer);
    generator.Reset();
    state.currentLimit = 0;
    state.inputBuffer[0] = '\0';
    state.isTyping = false;
  }
  if (key == KEY_ESCAPE) {
    state.inputBuffer[0] = '\0';
    state.isTyping = false;
  }
}
