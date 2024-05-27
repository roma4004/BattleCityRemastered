#include "../headers/BaseObj.h"

BaseObj::BaseObj(const Rectangle& rect, const int color, const float speed, const int health)
	: shape{rect}, _color(color), _speed(speed), _health(health)
{
}

BaseObj::~BaseObj() = default;

const Rectangle& BaseObj::GetShape() const { return shape; }

FPoint BaseObj::GetPos() const { return FPoint{shape.x, shape.y}; }

void BaseObj::SetPos(const FPoint& pos)
{
	shape.x = pos.x;
	shape.y = pos.y;
}

float BaseObj::GetRightSide() const { return shape.Right(); }

float BaseObj::GetBottomSide() const { return shape.Bottom(); }

float BaseObj::GetX() const { return shape.x; }

void BaseObj::SetX(const FPoint& pos) { shape.x = pos.x; }

float BaseObj::GetY() const { return shape.y; }

void BaseObj::SetY(const FPoint& pos) { shape.y = pos.y; }

float BaseObj::GetWidth() const { return shape.w; }

void BaseObj::SetWidth(const float width) { shape.w = width; }

float BaseObj::GetHeight() const { return shape.h; }

void BaseObj::SetHeight(const float height) { shape.h = height; }

void BaseObj::MoveX(const float i) { shape.x += i; }

void BaseObj::MoveY(const float i) { shape.y += i; }

float BaseObj::GetSpeed() const { return _speed; }

int BaseObj::GetColor() const { return _color; }

void BaseObj::SetColor(const int color) { _color = color; }

int BaseObj::GetHealth() const { return _health; }

void BaseObj::SetHealth(const int health) { _health = health; }

void BaseObj::SetIsAlive(const bool isAlive) { _isAlive = isAlive; }

bool BaseObj::GetIsAlive() const { return _isAlive; }

void BaseObj::TakeDamage(const int damage)
{
	_health -= damage;
	if (_health < 1)
	{
		_isAlive = false;
	}
}

bool BaseObj::GetIsPassable() const { return _isPassable; }

void BaseObj::SetIsPassable(const bool value) { _isPassable = value; }

bool BaseObj::GetIsDestructible() const { return _isDestructible; }

void BaseObj::SetIsDestructible(const bool value) { _isDestructible = value; }

bool BaseObj::GetIsPenetrable() const { return _isPenetrable; }

void BaseObj::SetIsPenetrable(const bool value) { _isPenetrable = value; }
