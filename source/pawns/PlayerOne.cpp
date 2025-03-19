#include "../../headers/pawns/PlayerOne.h"
#include "../../headers/Point.h"
#include "../../headers/behavior/MoveLikeTankBeh.h"
#include "../../headers/behavior/ShootingBeh.h"
#include "../../headers/components/EventSystem.h"
#include "../../headers/enums/Direction.h"
#include "../../headers/enums/GameMode.h"
#include "../../headers/interfaces/IInputProvider.h"
#include "../../headers/utils/TimeUtils.h"

#include <chrono>

PlayerOne::PlayerOne(const ObjRectangle& rect, const int color, const int health, std::shared_ptr<Window> window,
                     const Direction dir, const float speed, std::vector<std::shared_ptr<BaseObj>>* allObjects,
                     std::shared_ptr<EventSystem> events, std::string name, std::string fraction,
                     std::unique_ptr<IInputProvider> inputProvider, std::shared_ptr<BulletPool> bulletPool,
                     const GameMode gameMode, const int id, const int tier)
	: Tank{rect,
	       color,
	       health,
	       std::move(window),
	       dir,
	       speed,
	       allObjects,
	       events,
	       std::make_unique<MoveLikeTankBeh>(this, allObjects),
	       std::make_shared<ShootingBeh>(this, allObjects, events, std::move(bulletPool)),
	       std::move(name),
	       std::move(fraction),
	       gameMode,
	       id,
	       tier},
	  _inputProvider{std::move(inputProvider)} {}

PlayerOne::~PlayerOne() = default;

void PlayerOne::MoveTo(const float deltaTime, const Direction dir)
{
	SetDirection(dir);
	_moveBeh->Move(deltaTime);

	if (_gameMode == PlayAsHost)
	{
		_events->EmitEvent<const std::string&, const FPoint, const Direction>(
				"ServerSend_Pos", "Player" + std::to_string(GetId()), GetPos(), GetDirection());
	}
}

void PlayerOne::TickUpdate(const float deltaTime)
{
	Tank::TickUpdate(deltaTime);

	const auto playerKeys = _inputProvider->GetKeysStats();

	// move
	if (playerKeys.up)
	{
		MoveTo(deltaTime, UP);
	}
	else if (playerKeys.left)
	{
		MoveTo(deltaTime, LEFT);
	}
	else if (playerKeys.down)
	{
		MoveTo(deltaTime, DOWN);
	}
	else if (playerKeys.right)
	{
		MoveTo(deltaTime, RIGHT);
	}

	// shot
	if (playerKeys.shot && TimeUtils::IsCooldownFinish(_lastTimeFire, _fireCooldown))
	{
		Shot();
	}
}
