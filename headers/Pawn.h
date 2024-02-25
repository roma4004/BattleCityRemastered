#pragma once
#include "SDL.h"

#include "../headers/BaseObj.h"
#include "../headers/Environment.h"
#include "../headers/IDrawable.h"
#include "../headers/IMovable.h"
#include "../headers/PlayerKeys.h"

class Pawn : public BaseObj, public IMovable, public IDrawable
{
public:
  Pawn(int x, int y, int width, int height, int color, int speed);

  virtual ~Pawn();

  void Move(const Environment& env) override;

  void Draw(Environment& env) const override;

  virtual void KeyboardEvensHandlers(Environment& env, Uint32 eventType, SDL_Keycode key);

  [[nodiscard]] bool IsCollideWith(const SDL_Rect* self, const Pawn* other) const;

  [[nodiscard]] bool IsCanMove(const SDL_Rect* self, const Environment& env) const;

  PlayerKeys keyboardButtons;
};