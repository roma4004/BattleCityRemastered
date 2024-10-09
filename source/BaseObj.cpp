#include "../headers/BaseObj.h"
#include "../headers/Point.h"

BaseObj::BaseObj(const ObjRectangle& rect, const int color, const int health)
	: _color(color),
	  _health(health),
	  _shape{rect} {}

BaseObj::~BaseObj() = default;

const ObjRectangle& BaseObj::GetShape() const { return _shape; }

void BaseObj::SetShape(const ObjRectangle& shape) { _shape = shape; }

FPoint BaseObj::GetPos() const { return FPoint{_shape.x, _shape.y}; }

void BaseObj::SetPos(const FPoint& pos)
{
	_shape.x = pos.x;
	_shape.y = pos.y;
}

float BaseObj::GetRightSide() const { return _shape.Right(); }

float BaseObj::GetBottomSide() const { return _shape.Bottom(); }

float BaseObj::GetX() const { return _shape.x; }

void BaseObj::SetX(const FPoint& pos) { _shape.x = pos.x; }

float BaseObj::GetY() const { return _shape.y; }

void BaseObj::SetY(const FPoint& pos) { _shape.y = pos.y; }

float BaseObj::GetWidth() const { return _shape.w; }

void BaseObj::SetWidth(const float width) { _shape.w = width; }

float BaseObj::GetHeight() const { return _shape.h; }

void BaseObj::SetHeight(const float height) { _shape.h = height; }

void BaseObj::MoveX(const float i) { _shape.x += i; }

void BaseObj::MoveY(const float i) { _shape.y += i; }

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
