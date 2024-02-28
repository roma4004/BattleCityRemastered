#pragma once
#include "SDL.h"

#include "../headers/BaseObj.h"
#include "../headers/Environment.h"
#include "../headers/IDrawable.h"
#include "../headers/IMovable.h"
#include "../headers/PlayerKeys.h"

enum Direction
{
  UP = 0,
  LEFT = 1,
  DOWN = 2,
  RIGHT = 3
};

class Pawn : public BaseObj, public IMovable, public IDrawable
{
public:
  Pawn(int x, int y, int width, int height, int color, int speed, int health);

  virtual ~Pawn();

  void Move(Environment& env) override;

  void Draw(Environment& env) const override;

  virtual void KeyboardEvensHandlers(Environment& env, Uint32 eventType, SDL_Keycode key);

  [[nodiscard]] bool IsCollideWith(const SDL_Rect* self, const Pawn* other) const;

  [[nodiscard]] bool IsCanMove(const SDL_Rect* self, const Environment& env) const;

  void TickUpdate(Environment& env);
  
  virtual void Shot(Environment& env);

  [[nodiscard]] Direction GetDirection() const;

  void SetDirection(const Direction direction);
  
  
  PlayerKeys keyboardButtons;
private:
  Direction   _direction = Direction::UP;
  
};