#include "entities/pawns/Player.h"
#include "components/EventSystem.h"
#include "entities/pawns/PawnProperty.h"
#include "enums/Direction.h"
#include "interfaces/IInputProvider.h"
#include "interfaces/IMoveBeh.h"
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

	_fireCooldown = {std::chrono::milliseconds{500}};
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

void Player::Move(const Direction direction, const double deltaTime,
				  std::vector<std::shared_ptr<BaseObj>>& outCollisions)
{
	const Direction oldDir = GetDirection();
	const bool isNewDir = oldDir != direction;
	if (isNewDir)
	{
		SetDirection(direction);
	}

	if (const bool isMove = _moveBeh->Move(direction, deltaTime, outCollisions);
		isNewDir || isMove)
	{
		//TODO: let the animation manager work with string view
		_events->EmitEvent("AnimationTankUpdate", std::string(GetName()), GetPos(), GetDirection());
	}
}

void Player::TickUpdate(const double deltaTime)
{
	std::vector<std::shared_ptr<BaseObj>> outCollisions;
	const auto [up, left, down, right, shot] = _inputProvider->GetKeysStats();

	// move
	if (up)
	{
		Move(Direction::UP, deltaTime, outCollisions);
	}
	else if (left)
	{
		Move(Direction::LEFT, deltaTime, outCollisions);
	}
	else if (down)
	{
		Move(Direction::DOWN, deltaTime, outCollisions);
	}
	else if (right)
	{
		Move(Direction::RIGHT, deltaTime, outCollisions);
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
