#include "../../headers/pawns/Player.h"
#include "../../headers/Point.h"
#include "../../headers/behavior/MoveLikeTankBeh.h"
#include "../../headers/behavior/ShootingBeh.h"
#include "../../headers/components/EventSystem.h"
#include "../../headers/enums/Direction.h"
#include "../../headers/enums/GameMode.h"
#include "../../headers/interfaces/IInputProvider.h"
#include "../../headers/pawns/PawnProperty.h"
#include "../../headers/utils/TimeUtils.h"

#include <chrono>

Player::Player(PawnProperty pawnProperty, std::shared_ptr<BulletPool> bulletPool,
               std::unique_ptr<IInputProvider> inputProvider)
	: Tank{pawnProperty,
	       std::make_unique<MoveLikeTankBeh>(this, pawnProperty.allObjects),
	       std::make_shared<ShootingBeh>(this, pawnProperty.allObjects, pawnProperty.events, std::move(bulletPool))
	  },
	  _inputProvider{std::move(inputProvider)} {}

Player::~Player() = default;

void Player::MoveTo(const float deltaTime, const Direction dir)
{
	SetDirection(dir);
	_moveBeh->Move(deltaTime);

	if (_gameMode == PlayAsHost)
	{
		_events->EmitEvent<const std::string&, const FPoint, const Direction>(
				"ServerSend_Pos", _name, GetPos(), GetDirection());
	}
}

void Player::TickUpdate(const float deltaTime)
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
