# 0 "/home/breorkrat/Documents/projetos/Prime-Visualizer/src/modules/application.cppm"
# 0 "<built-in>"
# 0 "<command-line>"
# 1 "/usr/include/stdc-predef.h" 1 3
# 0 "<command-line>" 2
# 1 "/home/breorkrat/Documents/projetos/Prime-Visualizer/src/modules/application.cppm"
export module application;

import math_logic;
import game_state;
import renderer;

export class Application {
public:
  Application();
  void Run();

private:
  void HandleInput();
  void Update(float dt);

  GameState state;
  NumberGenerator generator;
  Renderer renderer;
};
