#include "../../headers/pawns/PlayerTwo.h"
#include "../../headers/EventSystem.h"
#include "../../headers/behavior/MoveLikeTankBeh.h"
#include "../../headers/behavior/ShootingBeh.h"
#include "../../headers/utils/TimeUtils.h"

#include <chrono>

PlayerTwo::PlayerTwo(const ObjRectangle& rect, const int color, const int health, std::shared_ptr<Window> window,
                     const Direction direction, const float speed, std::vector<std::shared_ptr<BaseObj>>* allObjects,
                     std::shared_ptr<EventSystem> events, std::string name, std::string fraction,
                     std::unique_ptr<IInputProvider> inputProvider, std::shared_ptr<BulletPool> bulletPool,
                     const GameMode gameMode, const int id)
	: Tank{rect,
	       color,
	       health,
	       std::move(window),
	       direction,
	       speed,
	       allObjects,
	       events,
	       std::make_unique<MoveLikeTankBeh>(this, allObjects),
	       std::make_shared<ShootingBeh>(this, allObjects, events, std::move(bulletPool)),
	       std::move(name),
	       std::move(fraction),
	       gameMode,
	       id},
	  _inputProvider{std::move(inputProvider)} {}

PlayerTwo::~PlayerTwo() = default;

void PlayerTwo::MoveTo(const float deltaTime, const Direction direction)
{
	SetDirection(direction);
	_moveBeh->Move(deltaTime);

	if (_gameMode == PlayAsHost)
	{
		_events->EmitEvent<const std::string&, const FPoint, const Direction>(
				"ServerSend_Pos", "Player" + std::to_string(GetId()), GetPos(), GetDirection());
	}
}

void PlayerTwo::TickUpdate(const float deltaTime)
{
	const auto playerKeys = _inputProvider->GetKeysStats();

	// move
	if (playerKeys.left)
	{
		MoveTo(deltaTime, LEFT);
	}
	else if (playerKeys.right)
	{
		MoveTo(deltaTime, RIGHT);
	}
	else if (playerKeys.up)
	{
		MoveTo(deltaTime, UP);
	}
	else if (playerKeys.down)
	{
		MoveTo(deltaTime, DOWN);
	}

	// shot
	if (playerKeys.shot && TimeUtils::IsCooldownFinish(_lastTimeFire, _fireCooldown))
	{
		Shot();
	}
}

void PlayerTwo::SendDamageStatistics(const std::string& author, const std::string& fraction)
{
	_events->EmitEvent<const std::string&, const std::string&>(_name + "Hit", author, fraction);

	if (GetHealth() < 1)
	{
		_events->EmitEvent<const std::string&, const std::string&>(_name + "Died", author, fraction);
	}
}

void PlayerTwo::TakeDamage(const int damage)
{
	Tank::TakeDamage(damage);

	if (_gameMode == PlayAsHost)
	{
		_events->EmitEvent<const std::string, const int>("ServerSend_Health", GetName(), GetHealth());
	}
}
