#include "entities/pawns/Player.h"
#include "components/EventSystem.h"
#include "entities/pawns/PawnProperty.h"
#include "enums/Direction.h"
#include "enums/GameMode.h"
#include "interfaces/IInputProvider.h"
#include "interfaces/IMoveBeh.h"
#include "utils/TimeUtils.h"

Player::Player(PawnProperty pawnProperty, const std::shared_ptr<BulletPool>& bulletPool,
			   std::unique_ptr<IInputProvider> inputProvider, GameConfig& gameConfig, const bool enableByDefault)
	: Tank{std::move(pawnProperty), bulletPool, gameConfig, enableByDefault}
	, _inputProvider{std::move(inputProvider)}
{
	if (enableByDefault)
	{
		Enable();
	}

	_shootTimer.cooldown = std::chrono::milliseconds{500};
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
		FPoint pos = GetPos();
		_events->EmitEvent("AnimationTankUpdate", GetName(), pos, _dir);

		if (_gameMode == GameMode::PlayAsHost)// NOTE: replication position to the client
		{
			_events->EmitEvent("ServerSend_Pos", _name, pos, _dir, _uuid);
		}
	}
}

void Player::TickUpdate(const double deltaTime)
{
	if (_shootTimer.isActive && _shootTimer.IsCooldownFinish())
	{
		_shootTimer.isActive = false;
	}

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

	if (_effects.isTouchTheIce && _moveBeh->ApplyMoveVelocity(deltaTime))
	{
		FPoint pos = GetPos();
		_events->EmitEvent("AnimationTankUpdate", GetName(), pos, _dir);

		if (_gameMode == GameMode::PlayAsHost)// NOTE: replication position to the client
		{
			_events->EmitEvent("ServerSend_Pos", _name, pos, _dir, _uuid);
		}
	}

	_effects.isTouchTheBushes = IsTouchBush();
	if (const bool isTouchTheIce = IsTouchIce();
		_effects.isTouchTheIce != isTouchTheIce)
	{
		_effects.isTouchTheIce = isTouchTheIce;
		_moveBeh->ResetVelocity();
	}

	if (!outCollisions.empty())
	{
		HandleBonusPickUp(outCollisions.front());
		outCollisions.clear();
	}

	// shot
	if (shot && !_shootTimer.isActive)
	{
		Shot();
	}
}
