#include "../headers/BaseObj.h"

#include <algorithm>

class Environment;

BaseObj::BaseObj(const Point& pos, const int width, const int height,
				 const int color, const int speed, int health)
	: _x(pos.x),
	  _y(pos.y),
	  _width(width),
	  _height(height),
	  _color(color),
	  _speed(speed),
	  _health(health) {}

BaseObj::~BaseObj() = default;

int BaseObj::GetX() const {
	return _x;
}

void BaseObj::SetX(const Point& pos) {
	_x = pos.x;
}

int BaseObj::GetY() const {
	return _y;
}

void BaseObj::SetY(const Point& pos) {
	_y = pos.y;
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

void BaseObj::MoveX(const int i) {
	_x += i;
}

void BaseObj::MoveY(const int i) {
	_y += i;
}

int BaseObj::GetSpeed() const {
	return _speed;
}

int BaseObj::GetColor() const {
	return _color;
}

void BaseObj::SetColor(const int color) {
	_color = color;
}

int BaseObj::GetHealth() const {
	return _health;
}

void BaseObj::SetHealth(const int health) {
	_health = health;
}

void BaseObj::SetIsAlive(const bool isAlive) {
	_isAlive = isAlive;
}

bool BaseObj::GetIsAlive() const {
	return _isAlive;
}

void BaseObj::TakeDamage(const int damage) {
	_health -= damage;
	if (_health < 1) {
		_isAlive = false;
	}
}
