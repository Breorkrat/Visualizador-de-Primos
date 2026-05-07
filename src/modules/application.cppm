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
