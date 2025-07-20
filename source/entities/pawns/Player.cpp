#include "entities/pawns/Player.h"
#include "behavior/MoveLikeTankBeh.h"
#include "behavior/ShootingBeh.h"
#include "entities/pawns/PawnProperty.h"
#include "enums/Direction.h"
#include "interfaces/IInputProvider.h"
#include "utils/TimeUtils.h"

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
		SetDirection(Direction::UP);
		std::ignore = Pawn::Move(deltaTime);
	}
	else if (left)
	{
		SetDirection(Direction::LEFT);
		std::ignore = Pawn::Move(deltaTime);
	}
	else if (down)
	{
		SetDirection(Direction::DOWN);
		std::ignore = Pawn::Move(deltaTime);
	}
	else if (right)
	{
		SetDirection(Direction::RIGHT);
		std::ignore = Pawn::Move(deltaTime);
	}

	// shot
	if (shot && TimeUtils::IsCooldownFinish(_lastTimeFire, _fireCooldown))
	{
		Shot();
	}
}
