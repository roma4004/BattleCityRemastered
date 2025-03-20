#include "../headers/BaseObj.h"
#include "../headers/Point.h"

BaseObj::BaseObj(BaseObjProperty baseObjProperty)
	: _color(baseObjProperty.color),
	  _health(baseObjProperty.health),
	  _id{baseObjProperty.id},
	  _name{std::move(baseObjProperty.name)},
	  _fraction{std::move(baseObjProperty.fraction)},
	  _shape{std::move(baseObjProperty.rect)} {}

BaseObj::BaseObj(ObjRectangle rect, const int color, const int health, const int id, std::string name,
                 std::string fraction)
	: _color(color),
	  _health(health),
	  _id{id},
	  _name{std::move(name)},
	  _fraction{std::move(fraction)},
	  _shape{std::move(rect)} {}

BaseObj::~BaseObj() = default;

ObjRectangle BaseObj::GetShape() const { return _shape; }

void BaseObj::SetShape(const ObjRectangle shape) { _shape = shape; }

std::string BaseObj::GetName() const { return _name; }

int BaseObj::GetId() const { return _id; }

std::string BaseObj::GetFraction() const { return _fraction; }

FPoint BaseObj::GetPos() const { return FPoint{.x = _shape.x, .y = _shape.y}; }

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

void BaseObj::SetHealth(const int health)
{
	_health = health;
	_isAlive = _health >= 1;
}

void BaseObj::SetIsAlive(const bool isAlive) { _isAlive = isAlive; }

bool BaseObj::GetIsAlive() const { return _isAlive; }

void BaseObj::TakeDamage(const int damage)
{
	_health -= damage;
	_isAlive = _health >= 1;
}

bool BaseObj::GetIsPassable() const { return _isPassable; }

void BaseObj::SetIsPassable(const bool value) { _isPassable = value; }

bool BaseObj::GetIsDestructible() const { return _isDestructible; }

void BaseObj::SetIsDestructible(const bool value) { _isDestructible = value; }

bool BaseObj::GetIsPenetrable() const { return _isPenetrable; }

void BaseObj::SetIsPenetrable(const bool value) { _isPenetrable = value; }
