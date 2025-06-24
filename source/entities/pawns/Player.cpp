#include "entities/pawns/Player.h"
#include "Point.h"
#include "behavior/MoveLikeTankBeh.h"
#include "behavior/ShootingBeh.h"
#include "components/EventSystem.h"
#include "entities/pawns/PawnProperty.h"
#include "enums/Direction.h"
#include "enums/GameMode.h"
#include "interfaces/IInputProvider.h"
#include "utils/TimeUtils.h"
#include <chrono>

Player::Player(PawnProperty pawnProperty, std::shared_ptr<BulletPool> bulletPool,
               std::unique_ptr<IInputProvider> inputProvider, const BonusEffectProperty effects = {})
	: Tank{pawnProperty,
	       std::make_unique<MoveLikeTankBeh>(this, pawnProperty.allObjects),
	       std::make_shared<ShootingBeh>(this, pawnProperty.allObjects, pawnProperty.events, std::move(bulletPool)),
	       effects
	  },
	  _inputProvider{std::move(inputProvider)} {}

Player::~Player() = default;

void Player::TickUpdate(const float deltaTime)
{
	if (_effects.isTimerActive)
	{
		return;
	}

	const auto [up, left, down, right, shot] = _inputProvider->GetKeysStats();

	// move
	if (up)
	{
		SetDirection(UP);
		Pawn::Move(deltaTime);
	}
	else if (left)
	{
		SetDirection(LEFT);
		Pawn::Move(deltaTime);
	}
	else if (down)
	{
		SetDirection(DOWN);
		Pawn::Move(deltaTime);
	}
	else if (right)
	{
		SetDirection(RIGHT);
		Pawn::Move(deltaTime);
	}

	// shot
	if (shot && TimeUtils::IsCooldownFinish(_lastTimeFire, _fireCooldown))
	{
		Shot();
	}
}
