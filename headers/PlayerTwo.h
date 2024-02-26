#pragma once

#include "../headers/Pawn.h"

class PlayerTwo final : public Pawn
{
public:
  PlayerTwo(int x, int y, int width, int height, int color, int speed, size_t id);
  
  void KeyboardEvensHandlers(Environment& env, Uint32 eventType, SDL_Keycode key) override;
};
