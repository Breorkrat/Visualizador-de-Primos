#include "raylib.h"

#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

//#include "raymath.h"

#include "resource_dir.h"	// utility header for SearchAndSetResourceDir

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#define TAM_BAR_X 10
#define TAM_BAR_Y 8

#define BAR_THICKNESS 1

typedef struct {
  double x;
  double y;
} doubleVector2;

typedef struct {
  doubleVector2 pos;      // Coordenadas cartesianas calculadas
  unsigned long long int p;   // Primo
  Color color;
} PrimePoint;

// Array dinâmico para remover o hard limit
typedef struct {
  PrimePoint *items;
  int count;
  int capacity;
} PrimeList;

// Informações da janela
typedef struct {
  int width;
  int height;
  int centrox;
  int centroy;
  char name[30];
} Window;

typedef struct {
  Color customStatic;
  Color customGradientCenter;
  Color customGradientEdge;
} ColorList;

typedef enum {
  COLOR_CALCULATED,
  COLOR_BREATHING,
  COLOR_CUSTOM_STATIC,
  COLOR_CUSTOM_GRADIENT,
  MAX_COLOR_MODES} ColorMode;

void addPrime(PrimeList*, unsigned long long int);
void sieveSegment(PrimeList*, unsigned long long int*, int);
int findStartIndexForCulling(Window, PrimeList*, float, int);
Color processColorMode(ColorMode, PrimePoint, ColorList, float, Color);
void changeMode(int, PrimeList*, float*, unsigned long long int*);
void genSegment(PrimeList*, unsigned long long int*, unsigned long long int, int);

int main (){
	// Tell the window to use vsync and work on high DPI displays
	SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_HIGHDPI | FLAG_WINDOW_RESIZABLE);

  Window window = {1920, 1080};
  window.centrox = window.width/2;
  window.centroy = window.height/2;
  strcpy(window.name, "Prime Visualizer");

  ColorList colors = {
    RED,
    WHITE,
    BLACK
  };

	// Create the window and OpenGL context
	InitWindow(window.width, window.height, window.name);

	// Utility function from resource_dir.h to find the resources folder and set it as the current working directory so we can load from it
	SearchAndSetResourceDir("resources");

  // Cria textura para renderizar em montes pela gpu
  Image img = GenImageColor(2, 2, WHITE);
  Texture2D dot = LoadTextureFromImage(img);
  UnloadImage(img);

  // Seta câmera da Raylib
  Camera2D camera = {0};
  camera.target = (Vector2){0, 0};
  camera.offset = (Vector2){ window.centrox, window.centroy };
  camera.rotation = 0.0f;
  camera.zoom = 1.0f;

  // 0: Cover screen. 1: See full spiral. 2: Free camera
  char zoomMode = 0;
  // Buffer para escrita de um número
  char inputBuffer[16] = {0};
  char isTyping = 0;

  char *colorSchemeName[] = {
    "Distance",
    "Breathing",
    "Custom static",
    "Custom Gradient"
  };

  ColorMode currentColorMode = COLOR_BREATHING;

  // Simula o efeito de "lentamente" renderizando primos, mesmo que eles já tenham sido calculados previamente
  float currentLimit = 0;
  char PPSlock = 0;
  float primesPerSecond = 0;
  char showStats = 0;
  char showControls = 0;
  char showFPS = 0;
  char showCursor = 0;

  // What multiples will be drawn. 0 = primes, any number is valid
  unsigned int divMode = 0;

  // 0 = Invisível. 1 = Solid color. 2 = Gradient color
  char colorPickerVisible = 0;

  // Inicializa lista de primos
  PrimeList primes = {0};
  addPrime(&primes, 2);
  unsigned long long int lastChecked = 0;
  sieveSegment(&primes, &lastChecked, 1000);
	
	// game loop
	while (!WindowShouldClose())		// run the loop until the user presses ESCAPE or presses the Close button on the window
	{
    // Checa se a janela mudou de resolução para recalcular posições
    if (IsWindowResized()) {
      window.width = GetScreenWidth();
      window.height = GetScreenHeight();
      camera.offset = (Vector2){ window.width / 2.0f, window.height / 2.0f };

      window.centrox = window.width/2;
      window.centroy = window.height/2; 
    }

    // Calcula PPS baseado no zoom da câmera
    if (!PPSlock) {
      primesPerSecond = 500.f + (50.0f / sqrt(camera.zoom)) * 1.0f;
      if (primesPerSecond > 30000.0f) primesPerSecond = 30000.0f;
    } 
    float moveSpeed = 10.0f / camera.zoom;

    if (IsKeyDown(KEY_W)) camera.target.y -= moveSpeed; 
    if (IsKeyDown(KEY_S)) camera.target.y += moveSpeed;
    if (IsKeyDown(KEY_A)) camera.target.x -= moveSpeed;
    if (IsKeyDown(KEY_D)) camera.target.x += moveSpeed;
    if (IsKeyDown(KEY_DOWN)){
      PPSlock = 1;
      primesPerSecond-=10;
    }
    if (IsKeyDown(KEY_UP)){
      PPSlock = 1;
      primesPerSecond+=10;
    }
    
    int character = GetCharPressed();

    // Adds key to inputBuffer and starts typing mode
    if (character >= '0' && character <= '9'){ 
      isTyping = 1;
      int len = strlen(inputBuffer);
      if (len < 15) {
        inputBuffer[len] = (char) character;
        inputBuffer[len+1] = '\0';
      }
    }
    if (isTyping) {
      // Se backspace, apaga última tecla
      if (IsKeyPressed(KEY_BACKSPACE)) {
        int len = strlen(inputBuffer);
        if (len > 0) inputBuffer[len-1] = '\0';
        else isTyping = false; // Se deletou tudo, sai do modo de edição
      }

      if (IsKeyPressed(KEY_ENTER)){
        divMode = atoi(inputBuffer);
        changeMode(divMode, &primes, &currentLimit, &lastChecked);
        // Reseta o buffer
        inputBuffer[0] = '\0';
        isTyping = false;
      }
      // Escape para cancelar
      if (IsKeyPressed(KEY_ESCAPE)){
        inputBuffer[0] = '\0';
        isTyping = false;
      }
    }

    // Fila de teclas para quando fps fica baixo
    int key = GetKeyPressed();
    while (key > 0) {
      if (key == KEY_R) {
        zoomMode = 0;
        camera.target = (Vector2){0, 0};
      }
      if (key == KEY_F) {
        zoomMode = 1;
        camera.target = (Vector2){0, 0};
      }
      if (key == KEY_ENTER) {
        if (!PPSlock) {
          PPSlock = 1;
          primesPerSecond = 0;
        } else {
          PPSlock = 0;
        }
      }
      if (key == KEY_C) {
        currentColorMode = (currentColorMode + 1) % MAX_COLOR_MODES;
      }
      if (key == KEY_P) {
        TakeScreenshot(TextFormat("prime_spiral_%d.png", (int) currentLimit));
      }
      if (key == KEY_F1) showControls = showControls ? 0 : 1;
      if (key == KEY_F2) showFPS = showFPS ? 0 : 1;
      if (key == KEY_F3) showStats = showStats ? 0 : 1;
      if (key == KEY_F4) showCursor = showCursor ? 0 : 1;
      if (key == KEY_TAB) {
        if (colorPickerVisible == 0) colorPickerVisible = 1;
        else if (colorPickerVisible == 1) colorPickerVisible = 2;
        else colorPickerVisible = 0;
      }
      if (key == KEY_RIGHT) {
        divMode++;
        changeMode(divMode, &primes, &currentLimit, &lastChecked);
      }
      if (key == KEY_LEFT && divMode > 0) {
        divMode--;
        changeMode(divMode, &primes, &currentLimit, &lastChecked);
      }

      key = GetKeyPressed();
    }

    float mouseWheelMovement = GetMouseWheelMove();
    if (mouseWheelMovement != 0) {
      zoomMode = 2;
      camera.zoom += (mouseWheelMovement * 0.1f * camera.zoom);
    }
    
    // Primos revelados
    currentLimit += primesPerSecond * GetFrameTime();

    // Se o zoom estiver próximo, 1.0f / camera.zoom será menor que 2px, então usa uma escala constante de 2 para fazer o pixel maior de perto
    float dotScale = fmaxf(2.0f, 1.0f / camera.zoom);

    // Limita para desenhar apenas alguns circulos por segundo
    if (currentLimit < 0) currentLimit = 0;
    if (currentLimit > primes.count) currentLimit = primes.count;

    // Se os primos já processados estão acabando, processa mais
    if (primes.count - currentLimit <= 1000) {
      if (divMode == 0) sieveSegment(&primes, &lastChecked, 1000000);
      else genSegment(&primes, &lastChecked, divMode, 100000);
    }


    // Zoom automático
    if (currentLimit > 0) {
      float maxRadius = (float)primes.items[(int)currentLimit - 1].p;
      switch(zoomMode) {
        case 0:
          camera.zoom = (window.width / 2.0f) / (maxRadius * 0.9f);
          break;
        case 1:
          camera.zoom = (window.height / 2.0f) / (maxRadius * 1.1f);
          break;
      }
    }

    // Drawing
		BeginDrawing();

		// Setup the back buffer for drawing (clear color and depth buffers)
		ClearBackground(BLACK);

    // Começa modo 2d para desenhar círculos
    BeginMode2D(camera);

    // LOD quando movendo camera, reduzindo lag ao mover renderizando apenas um a cada dez primos
    char isMoving = (IsKeyDown(KEY_W) || IsKeyDown(KEY_A) || IsKeyDown(KEY_S) || IsKeyDown(KEY_D) || GetMouseWheelMove() != 0);
    int step = 1;
    if (isMoving && currentLimit > 100000) step = 5;

    // Encontra com busca binária o primeiro primo que pode aparecer na câmera e o maior raio que a câmera vê
    Vector2 tl = GetScreenToWorld2D((Vector2){0, 0}, camera);
    Vector2 tr = GetScreenToWorld2D((Vector2){window.width, 0},camera);
    Vector2 bl = GetScreenToWorld2D((Vector2){0, window.height},camera);
    Vector2 br = GetScreenToWorld2D((Vector2){window.width, window.height}, camera);

    float minX = fminf(fminf(tl.x, tr.x), fminf(bl.x, br.x));
    float maxX = fmaxf(fmaxf(tl.x, tr.x), fmaxf(bl.x, br.x));
    float minY = fminf(fminf(tl.y, tr.y), fminf(bl.y, br.y));
    float maxY = fmaxf(fmaxf(tl.y, tr.y), fmaxf(bl.y, br.y));

    // Calcula o ponto da câmera mais próximo do centro 
    float cx = (minX > 0) ? minX : ((maxX < 0) ? maxX : 0);
    float cy = (minY > 0) ? minY : ((maxY < 0) ? maxY : 0);
    float rMin = sqrtf(cx*cx + cy*cy);

    // Calcula o ponto da câmera mais longe do centro, comparando os quadrados e tirando a raíz do maior
    float rMax = sqrtf(fmaxf(
      fmaxf(tl.x*tl.x + tl.y*tl.y, tr.x*tr.x + tr.y*tr.y), 
      fmaxf(bl.x*bl.x + bl.y*bl.y, br.x*br.x + br.y*br.y)));

    int start = findStartIndexForCulling(window, &primes, rMin, currentLimit);
    Color globalBreath = ColorFromHSV(fmodf(GetTime() * 50.0f, 360.0f), 0.7f, 1.0f);

    // Calcula o inverso do primo para acelerar cálculo de distanceRation, eliminando uma divisão por item
    float invMaxP = (currentLimit > 0) ? 1.0f / (float)primes.items[(int)currentLimit-1].p : 1.0f;

    for (int i = start; i < currentLimit; i+=step){
      // Para o desenho se os próximos círculos estão fora do ponto mais longe da tela
      if (primes.items[i].p > rMax) break;

      // Não desenha se o ponto está fora da área visível
      if (cx < minX || cx > maxX || cy < minY || cy > maxY) continue;

      // Proporção para os degradês
      float distanceRatio = (float)primes.items[i].p * invMaxP;

      Vector2 pos = {
        (float) primes.items[i].pos.x,
        (float) primes.items[i].pos.y
      };

      // Define como colorir os círculos
      Color drawColor = processColorMode(currentColorMode, primes.items[i], colors, distanceRatio, globalBreath);
      DrawTextureEx(dot, pos, 0.0f, dotScale, drawColor);
    }

    // Encerra modo de câmera para escrever textos
    EndMode2D();

    if (isTyping) {
      int boxX = window.width/2 - 150;
      int boxY = window.height/2 - 40;
      int boxWidth = 300;
      int boxHeight = 80;
      DrawRectangle(boxX, boxY, boxWidth, boxHeight, Fade(BLACK, 0.8f));
      DrawRectangleLines(boxX, boxY, boxWidth, boxHeight, RAYWHITE);
      DrawText("Jump to multiple of:", boxX + 20, boxY + 10, 20, GRAY);
      // Efeito de cursor piscando
      const char* cursor = (fmodf(GetTime(), 1.0f) < 0.5f) ? "_" : "";
      DrawText(TextFormat("%s%s", inputBuffer, cursor), boxX + 20, boxY + 40, 30, WHITE);

      DrawText("[Enter] Confirm [Esc] Cancel", boxX + 20, boxY + 80, 10, GRAY);
    }

    if (colorPickerVisible == 1) {
      GuiColorPicker((Rectangle){ window.width - 250, 50, 200, 200 }, "Pick a color", &colors.customStatic);
      currentColorMode = COLOR_CUSTOM_STATIC;
    } else if (colorPickerVisible == 2) {
      GuiColorPicker((Rectangle){ window.width - 250, 50, 200, 200 }, "Pick a color for the center", &colors.customGradientCenter);
      GuiColorPicker((Rectangle){ window.width - 250, 300, 200, 200 }, "Pick a color for the edge", &colors.customGradientEdge);
      currentColorMode = COLOR_CUSTOM_GRADIENT;
    }
    
    // Escreve estatísticas  na tela
    if (showStats) {
      unsigned long long int primoAtual = !currentLimit ? 0 : primes.items[(int)currentLimit-1].p;
      DrawText(TextFormat("Numbers rendered: %.0f", currentLimit), 10, 10, 20, WHITE);
      DrawText(TextFormat("Current number: %llu", primoAtual), 10, 30, 20, WHITE);
      DrawText(TextFormat("PPS: %.2f", primesPerSecond), 10, 50, 20, WHITE);
      DrawText(TextFormat("Numbers calculated: %d", primes.count), 10, 70, 20, WHITE);
      DrawText(TextFormat("Color scheme: %s", colorSchemeName[currentColorMode]), 10, 90, 20, WHITE);
      if (divMode == 0) {
        DrawText("Rendering prime numbers", 10, 110, 20, WHITE);
      } else {
        DrawText(TextFormat("Rendering multiples of %d", divMode), 10, 110, 20, WHITE);
      }
    }

    // Escreve os controles na tela
    if (showControls){
      DrawText("Scroll wheel to adjust the zoom", 10, window.height-30, 20, WHITE);
      DrawText("WASD to move the camera", 10, window.height-50, 20, WHITE);
      DrawText("R for camera mode Fill", 10, window.height-70, 20, WHITE);
      DrawText("F for camera mode (Full view)", 10, window.height-90, 20, WHITE);
      DrawText("Enter to pause", 10, window.height-110, 20, WHITE);
      DrawText("C to change color scheme", 10, window.height-130, 20, WHITE);
      DrawText("TAB to enter custom color modes", 10, window.height-150, 20, WHITE);
      DrawText("Arrow keys to change speed", 10, window.height-170, 20, WHITE);
      DrawText("P to take a screenshot", 10, window.height-190, 20, WHITE);
      DrawText("Left and right arrows to change number", 10, window.height-210, 20, WHITE);
      DrawText("[F1-F4] to show/hide UI elements", 10, window.height-230, 20, WHITE);
    }

    if (showFPS) DrawFPS(window.width - 100, 10);

    // Desenha cursor no centro da tela
    if (showCursor) {
      DrawRectangle(window.centrox-(TAM_BAR_X/2), window.centroy, TAM_BAR_X, BAR_THICKNESS, WHITE);
      DrawRectangle(window.centrox, window.centroy-(TAM_BAR_Y/2), BAR_THICKNESS, TAM_BAR_Y, WHITE);
    }

		// end the frame and get ready for the next one  (display frame, poll input, etc...)
		EndDrawing();
	}

	// destroy the window and cleanup the OpenGL context
	CloseWindow();
	return 0;
}

void sieveSegment(PrimeList *list, unsigned long long int *lastChecked, int segmentSize){
  // Limite inferior
  unsigned long long int low = *lastChecked + 1;
  // Limite superior
  unsigned long long int high = low + segmentSize - 1;

  // Cria um array de números para checagem e marca os que não são primos
  // 0 = primo, 1 = não primo
  char *isNotPrime = calloc(segmentSize, sizeof(char));

  // Se na primeira iteração, não usa lista conhecida para encontrar primos
  if (low == 1) {
    // Marca 1 como não primo
    isNotPrime[0] = 1;

    for (unsigned long long int p = 2; p*p <= high; p++) {
      // Se o número é primo, elimina seus múltiplos
      if (!isNotPrime[p - low]) 
        for (unsigned long long int j = p*p; j <= high; j += p)
         isNotPrime[j - low] = 1;
    }
  }

  // Itera pelos primos já calculados para "peneirar" a próxima demanda
  for (int i = 0; i < list->count; i++){
    unsigned long long int p = list->items[i].p;
    if (p*p > high) break;

    // Encontra o primeiro múltiplo de p no range (low, high) truncando a divisão
    unsigned long long int start = (low / p) * p;
    if (start < low) start += p;

    // Números menores que p*p já devem ter sido eliminados pelas iterações passadas
    if (start < p*p) start = p*p;
    
    for (unsigned long long int j = start; j <= high; j+=p) {
      // Ajusta posição pro array atual
      isNotPrime[j - low] = 1;
    }
  }

  // Adiciona os que não foram marcados à lista de primos
  for (unsigned int i = 0; i < segmentSize; i++) {
    if (!isNotPrime[i]) addPrime(list, i + low);
  }
  *lastChecked = high;
  free(isNotPrime);
}

void addPrime(PrimeList *list, unsigned long long int p){
  if (list->count >= list->capacity){
    // Se a lista estiver cheia, dobra a capacidade
    list->capacity = (list->capacity == 0) ? 1024 : list->capacity * 2;
    list->items = realloc(list->items, list->capacity * sizeof(PrimePoint));
  }

  double r = (double)p;
  // Mantem a precisão mesmo com ângulos absurdos usando o módulo
  double theta = fmod((double)p, 2*PI);

  list->items[list->count].pos.x = r * cos(theta);
  list->items[list->count].pos.y = -r * sin(theta);
  list->items[list->count].p = p;

  float hue = fmodf(list->items[list->count].p * 0.05f, 360.0f);
  list->items[list->count].color = ColorFromHSV(hue, 0.8f, 1.0f);

  list->count++;
}

// Usa busca binária para encontrar o primeiro primo que pode aparecer na câmera para Culling
int findStartIndexForCulling(Window window, PrimeList* list, float rMin, int currentLimit){
  if (currentLimit <= 0) return 0;
  int low = 0;
  int high = currentLimit - 1;
  int result = 0;

  while (low <= high){
    int mid = low + (high - low) / 2;
    // Se está dentro do alcance, pode ter um mais cedo
    if (list->items[mid].p >= rMin){
      result = mid;
      high = mid-1;
    } else {
      low = mid + 1;
    }
  }
  return result;
}

// Processa o modo de cor atual retornando a cor que deve ser pintada
Color processColorMode(ColorMode currentColorMode, PrimePoint currentPrime, ColorList colors, float distanceRatio, Color globalBreath){
  switch (currentColorMode) {
  case COLOR_CUSTOM_STATIC:
    return colors.customStatic;
    break;
  case COLOR_CUSTOM_GRADIENT:
    return ColorLerp(colors.customGradientCenter, colors.customGradientEdge, distanceRatio);
    break;
  case COLOR_CALCULATED:
    return currentPrime.color;
    break;
  case COLOR_BREATHING:
    return globalBreath;
    break;
  default: return WHITE;
  }
}

void changeMode(int mode, PrimeList* List, float* currentLimit, unsigned long long int* lastChecked){
  List->count = 0;
  *currentLimit = 0;
  *lastChecked = 0;
}

void genSegment(PrimeList* list, unsigned long long int* lastNumber, unsigned long long int n, int count){
  // Gera count números múltiplos de n
  for (int i = 0; i <= count; i++){
    *lastNumber += n;
    addPrime(list, *lastNumber);
  }
}
