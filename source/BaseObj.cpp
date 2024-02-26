#include "../headers/BaseObj.h"

BaseObj::BaseObj() = default;

BaseObj::BaseObj(const int x,const int y,const int width,const int height,const int color,const int speed, const size_t id)
        : _x(x),  _y(y),  _width(width),  _height(height), _color(color), _speed(speed), _id(id)
{

}

BaseObj::~BaseObj() = default;

int BaseObj::GetX() const {
  return _x;
}

void BaseObj::SetX(const int x) {
  _x = x;
}

int BaseObj::GetY() const {
  return _y;
}

void BaseObj::SetY(const int y) {
  _y = y;
}

int BaseObj::GetWidth() const {
  return _width;
}

void BaseObj::SetWidth(const int width) {
  _width = width;
}

int BaseObj::GetHeight() const {
  return _height;
}

void BaseObj::SetHeight(const int height) {
  _height = height;
}

void BaseObj::MoveX(const int i)
{
  _x += i;
}

void BaseObj::MoveY(const int i)
{
  _y += i;
}

int BaseObj::GetSpeed() const
{
  return _speed;
}

int BaseObj::GetColor() const
{
  return _color;
}

void BaseObj::SetColor(const int color)
{
  _color = color;
}