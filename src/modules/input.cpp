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
  // Auto PPS
  if (!state.ppsLock) {
    state.primesPerSecond =
        500.0f + (50.0f / std::sqrt(state.camera.zoom)) * 1.0f;
    if (state.primesPerSecond > 30000.0f)
      state.primesPerSecond = 30000.0f;
  }

  // Camera Movement
  float moveSpeed = 10.0f / state.camera.zoom;
  if (IsKeyDown(KEY_W))
    state.camera.y -= moveSpeed;
  if (IsKeyDown(KEY_S))
    state.camera.y += moveSpeed;
  if (IsKeyDown(KEY_A))
    state.camera.x -= moveSpeed;
  if (IsKeyDown(KEY_D))
    state.camera.x += moveSpeed;

  // Setas de velocidade
  if (IsKeyDown(KEY_UP)) {
    state.ppsLock = true;
    state.primesPerSecond += 10.0f;
  }
  if (IsKeyDown(KEY_DOWN)) {
    state.ppsLock = true;
    state.primesPerSecond -= 10.0f;
  }

  // Scroll
  float mouseWheel = GetMouseWheelMove();
  if (mouseWheel != 0) {
    state.camera.zoomMode = 2;
    state.camera.zoom += (mouseWheel * 0.1f * state.camera.zoom);
  }
}

void InputManager::HandlePressed(GameState &state, NumberGenerator &generator) {
  // Key presses queue
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

      if (key == KEY_R) {
        state.camera.zoomMode = 0;
        state.camera.x = 0.0f;
        state.camera.y = 0.0f;
        state.camera.zoom = 1.0f;
      }
      if (key == KEY_F) {
        state.camera.zoomMode = 1;
        state.camera.x = 0.0f;
        state.camera.y = 0.0f;
        state.camera.zoom = 1.0f;
      }

      // Change color mode
      if (key == KEY_C)
        state.colorMode =
            static_cast<ColorMode>((static_cast<int>(state.colorMode) + 1) % 4);

      // Custom colors
      if (key == KEY_TAB) {
        // Alters between 0 (Hidden), 1 (Solid) and 2 (Gradient)
        state.colorPickerVisible = (state.colorPickerVisible + 1) % 3;
      }

      // Pause/unpause rendering. Only when not typing
      if (key == KEY_ENTER && !state.isTyping) {
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
          // Cleans list and generate primes within 50 million integers
          generator.Reset();
          generator.GeneratePrimesInRange(50000000);

          // Joga todos de uma vez só na tela (ignora a animação)
          state.currentLimit = generator.GetPoints().size();
          state.showFPS = true; // Força o FPS na tela

          // Bota a câmera num zoom ideal para ver o borrão inteiro
          state.camera.x = 0.0f;
          state.camera.y = 0.0f;
          state.camera.zoom = 0.05f;
        } else {
          generator.Reset();
          state.currentLimit = 0;
          state.camera.zoom = 1.0f;
        }
      }
    }
    key = GetKeyPressed();
  }

  // A captura do caractere para começar a digitar acontece fora da fila do
  // GetKeyPressed
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
