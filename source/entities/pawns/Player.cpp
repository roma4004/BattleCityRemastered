#include "entities/pawns/Player.h"
#include "entities/pawns/PawnProperty.h"
#include "enums/Direction.h"
#include "interfaces/IInputProvider.h"
#include "utils/TimeUtils.h"

Player::Player(PawnProperty pawnProperty, const std::shared_ptr<BulletPool>& bulletPool,
			   std::unique_ptr<IInputProvider> inputProvider, const bool enableByDefault)
	: Tank{std::move(pawnProperty), bulletPool, enableByDefault}
	, _inputProvider{std::move(inputProvider)}
{
	if (enableByDefault)
	{
		Enable();
	}
}

Player::~Player() = default;

void Player::Enable()
{
	Tank::Enable();

	_inputProvider->Enable();
}

void Player::Disable() const
{
	Tank::Disable();

	_inputProvider->Disable();
}

void Player::TickUpdate(const double deltaTime)
{
	if (_effects.isTimerActive)//TODO: disable tick update when active
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
