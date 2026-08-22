#include "entities/pawns/Player.h"
#include "behavior/MoveLikeTankBeh.h"
#include "components/EventSystem.h"
#include "components/events/AnimationRenderEvents.h"
#include "components/events/ReplicationEvents.h"
#include "entities/pawns/PawnProperty.h"
#include "enums/Direction.h"
#include "enums/GameMode.h"
#include "interfaces/IInputProvider.h"
#include "interfaces/IMoveBeh.h"

Player::Player(PawnProperty pawnProperty, const std::shared_ptr<BulletPool>& bulletPool,
			   std::unique_ptr<IInputProvider> inputProvider, GameConfig& gameConfig)
	: Tank{std::move(pawnProperty), bulletPool, gameConfig}
	, _inputProvider{std::move(inputProvider)}
{
	_inputProvider->Enable();

	_shootTimer.cooldown = std::chrono::milliseconds{500};
}

Player::~Player() = default;

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
		const FPoint pos = GetPos();
		_events->EmitEvent(AnimationTankUpdateEvent{.name = GetName(), .pos = pos, .dir = _dir});

		if (IsHost(_gameMode))
		{
			_events->EmitEvent(PosChangedEvent{.who = _name, .pos = pos, .dir = _dir, .uuid = _uuid});
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

	if (_effects.isTouchTheIce)
	{
		if (auto* moveBeh = dynamic_cast<MoveLikeTankBeh*>(_moveBeh.get());
			moveBeh && moveBeh->ApplyMoveVelocity(deltaTime))
		{
			const FPoint pos = GetPos();
			_events->EmitEvent(AnimationTankUpdateEvent{.name = GetName(), .pos = pos, .dir = _dir});

			if (IsHost(_gameMode))
			{
				_events->EmitEvent(PosChangedEvent{.who = _name, .pos = pos, .dir = _dir, .uuid = _uuid});
			}

		}
	}

	_effects.isTouchTheBushes = IsTouchBush();
	if (const bool isTouchTheIce = IsTouchIce();
		_effects.isTouchTheIce != isTouchTheIce)
	{
		_effects.isTouchTheIce = isTouchTheIce;
		if (auto* moveBeh = dynamic_cast<MoveLikeTankBeh*>(_moveBeh.get()))
		{
			moveBeh->ResetVelocity();
		}
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
