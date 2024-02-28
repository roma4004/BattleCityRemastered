#pragma once


class BaseObj
{
public:
  BaseObj(int x, int y, int width, int height, int color, int speed, int health);

  virtual ~BaseObj();

  [[nodiscard]] int GetX() const;
  void SetX(int x);

  [[nodiscard]] int GetY() const;
  void SetY(int y);

  [[nodiscard]] int GetWidth() const;
  void SetWidth(int width);

  [[nodiscard]] int GetHeight() const;
  void SetHeight(int height);

  void MoveX(int i);
  void MoveY(int i);

  [[nodiscard]] int GetSpeed() const;

  [[nodiscard]] int GetColor() const;
  void SetColor(int color);
  
  [[nodiscard]] int GetHealth() const;
  void SetHealth(int health);

  [[nodiscard]] bool GetIsAlive () const;
  void SetIsAlive (bool isAlive);

  virtual void TakeDamage(int damage);

private:
  int         _x{};
  int         _y{};
  int         _width{};
  int         _height{};
  int         _color{};
  int         _speed{};
  int         _health{};
  bool        _isAlive = true;

};
