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
	std::vector<std::shared_ptr<BaseObj>> outCollisions;
	const auto [up, left, down, right, shot] = _inputProvider->GetKeysStats();

	// move
	if (up)
	{
		const Direction oldDir = GetDirection();
		const bool isNewDir = oldDir != Direction::UP;
		if (isNewDir)
		{
			SetDirection(Direction::UP);
		}

		std::ignore = Pawn::Move(outCollisions, deltaTime, isNewDir);
	}
	else if (left)
	{
		const Direction oldDir = GetDirection();
		const bool isNewDir = oldDir != Direction::LEFT;
		if (isNewDir)
		{
			SetDirection(Direction::LEFT);
		}

		std::ignore = Pawn::Move(outCollisions, deltaTime, isNewDir);
	}
	else if (down)
	{
		const Direction oldDir = GetDirection();
		const bool isNewDir = oldDir != Direction::DOWN;
		if (isNewDir)
		{
			SetDirection(Direction::DOWN);
		}

		std::ignore = Pawn::Move(outCollisions, deltaTime, isNewDir);
	}
	else if (right)
	{
		const Direction oldDir = GetDirection();
		const bool isNewDir = oldDir != Direction::RIGHT;
		if (isNewDir)
		{
			SetDirection(Direction::RIGHT);
		}

		std::ignore = Pawn::Move(outCollisions, deltaTime, isNewDir);
	}

	if (!outCollisions.empty())
	{
		HandleBonusPickUp(outCollisions.front());
		outCollisions.clear();
	}

	// shot
	if (shot && TimeUtils::IsCooldownFinish(_lastTimeFire, _fireCooldown))
	{
		Shot();
	}
}
