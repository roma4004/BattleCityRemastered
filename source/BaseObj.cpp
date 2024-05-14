#include "../headers/BaseObj.h"

struct Environment;

BaseObj::BaseObj(const FPoint& pos, const float width, const float height, const int color, const float speed, const int health,
				 Environment* env)
	: shape(pos.x, pos.y, width, height), _color(color), _speed(speed), _health(health), _env(env)
{
}

BaseObj::~BaseObj() = default;

FPoint BaseObj::GetPos() const
{
	return shape.pos;
}

void BaseObj::SetPos(const FPoint& pos)
{
	shape.pos = pos;
}

float BaseObj::GetRightSide() const
{
	return shape.Right();
}

float BaseObj::GetBottomSide() const
{
	return shape.Bottom();
}

float BaseObj::GetX() const
{
	return shape.pos.x;
}

void BaseObj::SetX(const FPoint& pos)
{
	shape.pos.x = pos.x;
}

float BaseObj::GetY() const
{
	return shape.pos.y;
}

void BaseObj::SetY(const FPoint& pos)
{
	shape.pos.y = pos.y;
}

float BaseObj::GetWidth() const
{
	return shape.width;
}

void BaseObj::SetWidth(const float width)
{
	shape.width = width;
}

float BaseObj::GetHeight() const
{
	return shape.height;
}

void BaseObj::SetHeight(const float height)
{
	shape.height = height;
}

void BaseObj::MoveX(const float i)
{
	shape.pos.x += i;
}

void BaseObj::MoveY(const float i)
{
	shape.pos.y += i;
}

float BaseObj::GetSpeed() const
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

int BaseObj::GetHealth() const
{
	return _health;
}

void BaseObj::SetHealth(const int health)
{
	_health = health;
}

void BaseObj::SetIsAlive(const bool isAlive)
{
	_isAlive = isAlive;
}

bool BaseObj::GetIsAlive() const
{
	return _isAlive;
}

void BaseObj::TakeDamage(const int damage)
{
	_health -= damage;
	if (_health < 1)
	{
		_isAlive = false;
	}
}

bool BaseObj::GetIsPassable() const
{
	return _isPassable;
}

void BaseObj::SetIsPassable(const bool value)
{
	_isPassable = value;
}

bool BaseObj::GetIsDestructible() const
{
	return _isDestructible;
}

void BaseObj::SetIsDestructible(const bool value)
{
	_isDestructible = value;
}

bool BaseObj::GetIsPenetrable() const
{
	return _isPenetrable;
}

void BaseObj::SetIsPenetrable(const bool value)
{
	_isPenetrable = value;
}
