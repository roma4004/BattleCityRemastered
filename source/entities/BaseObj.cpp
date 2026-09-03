#include "entities/BaseObj.h"
#include "geometry/Point.h"
#include "entities/BaseObjProperty.h"

BaseObj::BaseObj(BaseObjProperty baseObjProperty, const CollisionTags collision)
	: _health(baseObjProperty.health)
	, _collision{collision}
	, _uuid{baseObjProperty.uuid}
	, _faction{baseObjProperty.faction}
	, _rect{baseObjProperty.rect}
{}

BaseObj::~BaseObj() = default;


const ObjRectangle& BaseObj::GetRect() const { return _rect; }

void BaseObj::SetRect(const ObjRectangle rect) { _rect = rect; }

Uuid BaseObj::GetUuid() const { return _uuid; }

void BaseObj::SetId(const Uuid uuid) { _uuid = uuid; }

Faction BaseObj::GetFaction() const { return _faction; }

FPoint BaseObj::GetPos() const { return FPoint{.x = _rect.x, .y = _rect.y}; }

void BaseObj::SetPos(const FPoint& pos)
{
	_rect.x = pos.x;
	_rect.y = pos.y;
}

double BaseObj::GetRightSide() const { return _rect.Right(); }

double BaseObj::GetBottomSide() const { return _rect.Bottom(); }

double BaseObj::GetX() const { return _rect.x; }

void BaseObj::SetX(const FPoint& pos) { _rect.x = pos.x; }

double BaseObj::GetY() const { return _rect.y; }

void BaseObj::SetY(const FPoint& pos) { _rect.y = pos.y; }

double BaseObj::GetWidth() const { return _rect.w; }

void BaseObj::SetWidth(const double width) { _rect.w = width; }

double BaseObj::GetHeight() const { return _rect.h; }

void BaseObj::SetHeight(const double height) { _rect.h = height; }

void BaseObj::MoveX(const double i) { _rect.x += i; }

void BaseObj::MoveY(const double i) { _rect.y += i; }

int BaseObj::GetHealth() const { return _health; }

//NOTE: health alone never buries anyone - the client learns of a death from DespawnedEvent, and the
//host decides it in TakeDamage
void BaseObj::SetHealth(const int health) { _health = health; }

void BaseObj::SetIsAlive(const bool isAlive) { _isAlive = isAlive; }

bool BaseObj::GetIsAlive() const { return _isAlive; }

void BaseObj::EmitDamageStatistics(Author) {}

void BaseObj::EmitDeathStatistics(Author) {}

void BaseObj::TakeDamage(const unsigned int damage, const Author author)
{
	//NOTE: a corpse lingers in _allObjects until DisposeDeadObject on PostTickUpdate - it is still in
	//the blast radius of the next shot, and hitting it again would report a second death
	if (!_isAlive)
	{
		return;
	}

	_health -= static_cast<int>(damage);
	_isAlive = _health > 0;

	EmitDamageStatistics(author);

	if (!_isAlive)
	{
		EmitDeathStatistics(author);
	}
}

bool BaseObj::GetIsPassable() const { return _collision.passable; }

bool BaseObj::GetIsDestructible() const { return _collision.destructible; }

bool BaseObj::GetIsPenetrable() const { return _collision.penetrable; }
