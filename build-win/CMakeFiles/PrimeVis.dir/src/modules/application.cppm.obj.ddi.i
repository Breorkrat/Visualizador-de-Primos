# 0 "/home/breorkrat/Apps/Prime-Visualizer-CPP/src/modules/application.cppm"
# 0 "<built-in>"
# 0 "<command-line>"
# 1 "/home/breorkrat/Apps/Prime-Visualizer-CPP/src/modules/application.cppm"
export module application;

import math_logic;
import game_state;
import renderer;
import input;

export class Application {
public:
  Application();
  void Run();

private:
  void Update(float dt);

  GameState state;
  NumberGenerator generator;
  Renderer renderer;
  InputManager inputManager;
};
