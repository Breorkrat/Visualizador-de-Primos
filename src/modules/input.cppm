export module input;
import game_state;
import math_logic;

export class InputManager {
public:
  void ProcessInputs(GameState &state, NumberGenerator &generator);

private:
  void HandleContinuous(GameState &state);
  void HandlePressed(GameState &state, NumberGenerator &generator);
  void HandleTyping(GameState &state, NumberGenerator &generator, int key);
};
