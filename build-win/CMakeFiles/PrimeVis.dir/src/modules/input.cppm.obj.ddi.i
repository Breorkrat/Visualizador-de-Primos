# 0 "/home/breorkrat/Apps/Prime-Visualizer-CPP/src/modules/input.cppm"
# 0 "<built-in>"
# 0 "<command-line>"
# 1 "/home/breorkrat/Apps/Prime-Visualizer-CPP/src/modules/input.cppm"
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
