#include "entities/BaseObj.h"
#include "Point.h"
#include "entities/BaseObjProperty.h"
#include "utils/UuidUtils.h"
#include <utility>

BaseObj::BaseObj(BaseObjProperty baseObjProperty)
	: _health(baseObjProperty.health)
	, _uuid{baseObjProperty.uuid}
	, _name{std::move(baseObjProperty.name)}
	, _fraction{std::move(baseObjProperty.fraction)}
	, _rect{baseObjProperty.rect}
{
	_nameWithUuid = _name + UuidUtils::GetStringUuid(_uuid);
}

//Copy ctor
BaseObj::BaseObj(const BaseObj& other) = default;

//Move ctor
BaseObj::BaseObj(BaseObj&& other) noexcept
	: _health(std::exchange(other._health, 0))
	, _uuid(other._uuid)
	, _name(other._name)
	, _nameWithUuid(other._nameWithUuid)
	, _fraction(other._fraction)
	, _rect(other._rect) {}

BaseObj::~BaseObj() = default;

//Copy assignment
BaseObj& BaseObj::operator=(const BaseObj& other)
{
	if (this != &other)
	{
		_health = other._health;
		_uuid = other._uuid;
		_name = other._name;
		_nameWithUuid = other._nameWithUuid;
		_fraction = other._fraction;
		_rect = other._rect;
	}

	return *this;
}

//Move assignment
BaseObj& BaseObj::operator=(BaseObj&& other) noexcept
{
	if (this != &other)
	{
		_health = std::exchange(other._health, 0);
		_uuid = other._uuid;
		_name = other._name;
		_nameWithUuid = other._nameWithUuid;
		_fraction = other._fraction;
		_rect = other._rect;
	}

	return *this;
}

ObjRectangle BaseObj::GetRect() const { return _rect; }

void BaseObj::SetRect(const ObjRectangle rect) { _rect = rect; }

std::string_view BaseObj::GetName() const { return _name; }

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

int BaseObj::GetHealth() const { return _health; }

void BaseObj::SetHealth(const int health)
{
	_health = health;
	_isAlive = _health > 0;
}

void BaseObj::SetIsAlive(const bool isAlive) { _isAlive = isAlive; }

bool BaseObj::GetIsAlive() const { return _isAlive; }

void BaseObj::TakeDamage(const unsigned int damage, const std::string& /*damageAuthor*/, const std::string& /*damageFraction*/)
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
