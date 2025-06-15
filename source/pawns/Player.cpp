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
               std::unique_ptr<IInputProvider> inputProvider, const BonusEffectProperty effects)
	: Tank{pawnProperty,
	       std::make_unique<MoveLikeTankBeh>(this, pawnProperty.allObjects),
	       std::make_shared<ShootingBeh>(this, pawnProperty.allObjects, pawnProperty.events, std::move(bulletPool)),
	       effects
	  },
	  _inputProvider{std::move(inputProvider)} {}

Player::~Player() = default;

void Player::Move(const Direction dir, const float deltaTime)
{
	if (_effects.isTimerActive)
	{
		return;
	}

	SetDirection(dir);
	if (_moveBeh->Move(deltaTime))
	{
		UpdateAnimationFrame();
	}

	if (_gameMode == PlayAsHost)
	{
		_events->EmitEvent<const std::string&, const FPoint, const Direction, const buuid&>(
				"ServerSend_Pos", _name, GetPos(), GetDirection(), _uuid);
	}
}

void Player::TickUpdate(const float deltaTime)
{
	const auto [up, left, down, right, shot] = _inputProvider->GetKeysStats();

	// move
	if (up)
	{
		Move(UP, deltaTime);
	}
	else if (left)
	{
		Move(LEFT, deltaTime);
	}
	else if (down)
	{
		Move(DOWN, deltaTime);
	}
	else if (right)
	{
		Move(RIGHT, deltaTime);
	}

	// shot
	if (shot && TimeUtils::IsCooldownFinish(_lastTimeFire, _fireCooldown))
	{
		Shot();
	}
}
