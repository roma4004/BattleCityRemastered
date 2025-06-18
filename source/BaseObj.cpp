#include "../headers/BaseObj.h"
#include "../headers/BaseObjProperty.h"
#include "../headers/Point.h"
#include "../headers/utils/UuidUtils.h"

BaseObj::BaseObj(BaseObjProperty baseObjProperty)
	: _color(baseObjProperty.color),
	  _health(baseObjProperty.health),
	  _uuid{baseObjProperty.uuid},
	  _name{std::move(baseObjProperty.name)},
	  _fraction{std::move(baseObjProperty.fraction)},
	  _rect{std::move(baseObjProperty.rect)}
{
	_nameWithUuid = _name + UuidUtils::GetStringUuid(_uuid);
}

//Deprecated
BaseObj::BaseObj(const ObjRectangle rect, const int color, const int health, const buuid uuid, std::string name,
                 std::string fraction)
	: _color(color),
	  _health(health),
	  _uuid{uuid},
	  _name{std::move(name)},
	  _fraction{std::move(fraction)},
	  _rect{rect}
{
	_nameWithUuid = _name + UuidUtils::GetStringUuid(_uuid);
}

BaseObj::~BaseObj() = default;

ObjRectangle BaseObj::GetRect() const { return _rect; }

void BaseObj::SetRect(const ObjRectangle rect) { _rect = rect; }

const std::string& BaseObj::GetName() const { return _name; }

using buuid = boost::uuids::uuid;
buuid BaseObj::GetUuid() const { return _uuid; }

void BaseObj::SetId(const buuid uuid) { _uuid = uuid; }

std::string BaseObj::GetFraction() const { return _fraction; }

FPoint BaseObj::GetPos() const { return FPoint{.x = _rect.x, .y = _rect.y}; }

void BaseObj::SetPos(const FPoint& pos)
{
	_rect.x = pos.x;
	_rect.y = pos.y;
}

float BaseObj::GetRightSide() const { return _rect.Right(); }

float BaseObj::GetBottomSide() const { return _rect.Bottom(); }

float BaseObj::GetX() const { return _rect.x; }

void BaseObj::SetX(const FPoint& pos) { _rect.x = pos.x; }

float BaseObj::GetY() const { return _rect.y; }

void BaseObj::SetY(const FPoint& pos) { _rect.y = pos.y; }

float BaseObj::GetWidth() const { return _rect.w; }

void BaseObj::SetWidth(const float width) { _rect.w = width; }

float BaseObj::GetHeight() const { return _rect.h; }

void BaseObj::SetHeight(const float height) { _rect.h = height; }

void BaseObj::MoveX(const float i) { _rect.x += i; }

void BaseObj::MoveY(const float i) { _rect.y += i; }

int BaseObj::GetColor() const { return _color; }

void BaseObj::SetColor(const int color) { _color = color; }

int BaseObj::GetHealth() const { return _health; }

void BaseObj::SetHealth(const int health)
{
	_health = health;
	_isAlive = _health > 0;
}

void BaseObj::SetIsAlive(const bool isAlive) { _isAlive = isAlive; }

bool BaseObj::GetIsAlive() const { return _isAlive; }

void BaseObj::TakeDamage(const int damage)
{
	_health -= damage;
	_isAlive = _health > 0;
}

bool BaseObj::GetIsPassable() const { return _isPassable; }

void BaseObj::SetIsPassable(const bool value) { _isPassable = value; }

bool BaseObj::GetIsDestructible() const { return _isDestructible; }

void BaseObj::SetIsDestructible(const bool value) { _isDestructible = value; }

bool BaseObj::GetIsPenetrable() const { return _isPenetrable; }

void BaseObj::SetIsPenetrable(const bool value) { _isPenetrable = value; }
