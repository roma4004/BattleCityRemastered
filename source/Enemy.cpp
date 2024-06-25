#include "../headers/Enemy.h"
#include "../headers/CoopAI.h"
#include "../headers/EventSystem.h"
#include "../headers/MoveLikeAIBeh.h"
#include "../headers/PlayerOne.h"
#include "../headers/PlayerTwo.h"

#include <algorithm>
#include <chrono>

Enemy::Enemy(const Rectangle& rect, const int color, const float speed, const int health, int* windowBuffer,
             const UPoint windowSize, std::vector<std::shared_ptr<BaseObj>>* allPawns,
             std::shared_ptr<EventSystem> events, std::string name, std::string fraction)
	: Tank{rect,
	       color,
	       health,
	       windowBuffer,
	       windowSize,
	       allPawns,
	       std::move(events),
	       std::make_shared<MoveLikeAIBeh>(windowSize, speed, this, allPawns)},
	  distDirection(0, 3), distTurnRate(1, 5), _name{std::move(name)}, _fraction{std::move(fraction)}
{
	BaseObj::SetIsPassable(false);
	BaseObj::SetIsDestructible(true);
	BaseObj::SetIsPenetrable(false);

	std::random_device rd;
	gen = std::mt19937(std::chrono::high_resolution_clock::now().time_since_epoch().count() + rd());

	// subscribe
	if (_events == nullptr)
	{
		return;
	}

	_events->AddListener<float>("TickUpdate", _name, [this](const float deltaTime) { this->TickUpdate(deltaTime); });

	_events->AddListener("Draw", _name, [this]() { this->Draw(); });
}

Enemy::~Enemy()
{
	// unsubscribe
	if (_events == nullptr)
	{
		return;
	}

	_events->RemoveListener<float>("TickUpdate", _name);

	_events->RemoveListener("Draw", _name);

	_events->EmitEvent(_name + "_Died");
}

bool Enemy::IsCollideWith(const Rectangle& r1, const Rectangle& r2)
{

	// Check if one rectangle is to the right of the other
	if (r1.x > r2.x + r2.w || r2.x > r1.x + r1.w)
	{
		return false;
	}

	// Check if one rectangle is above the other
	if (r1.y > r2.y + r2.h || r2.y > r1.y + r1.h)
	{
		return false;
	}

	// If neither of the above conditions are met, the rectangles overlap
	return true;
}

bool Enemy::IsPlayerVisible(const std::vector<std::weak_ptr<BaseObj>>& obstacles, Direction dir)
{
	if (!obstacles.empty())
	{
		std::shared_ptr<BaseObj> isPlayerTeamSeen = obstacles.front().lock();
		if (dynamic_cast<PlayerOne*>(isPlayerTeamSeen.get())
		    || dynamic_cast<PlayerTwo*>(isPlayerTeamSeen.get())
		    || dynamic_cast<CoopAI*>(isPlayerTeamSeen.get()))
		{
			return true;
		}
	}

	return false;
}

void Enemy::MayShoot(Direction dir)
{
	const FPoint windowSize = {static_cast<float>(_windowSize.x), static_cast<float>(_windowSize.y)};
	const float tankHalfWidth = GetWidth() / 2.f;
	const float tankHalfHeight = GetHeight() / 2.f;
	const float bulletHalfWidth = GetBulletWidth() / 2.f;
	const float bulletHalfHeight = GetBulletHeight() / 2.f;
	std::vector<Rectangle> LOScheck{
			/*up, left, down, right*/
			{shape.x + tankHalfWidth - bulletHalfWidth, 0, GetBulletWidth(), shape.y},
			{0, shape.y + tankHalfHeight - bulletHalfHeight, shape.x, GetBulletHeight()},
			{shape.x + tankHalfWidth - bulletHalfWidth, shape.y + GetHeight(), GetBulletWidth(), windowSize.y},
			{shape.x + GetWidth(), shape.y + tankHalfHeight - bulletHalfHeight, windowSize.x, GetBulletHeight()}};

	// parse all seen in LOS (line of sight) obj
	std::vector<std::weak_ptr<BaseObj>> upSideObstacles{};
	std::vector<std::weak_ptr<BaseObj>> leftSideObstacles{};
	std::vector<std::weak_ptr<BaseObj>> downSideObstacles{};
	std::vector<std::weak_ptr<BaseObj>> rightSideObstacles{};
	for (std::shared_ptr<BaseObj>& pawn: *_allPawns)
	{
		if (this == pawn.get())
		{
			continue;
		}

		if (!pawn->GetIsPassable())
		{
			if (IsCollideWith(LOScheck[UP], pawn->GetShape()))
			{
				upSideObstacles.emplace_back(std::weak_ptr(pawn));
			}
			if (IsCollideWith(LOScheck[LEFT], pawn->GetShape()))
			{
				leftSideObstacles.emplace_back(std::weak_ptr(pawn));
			}
			if (IsCollideWith(LOScheck[DOWN], pawn->GetShape()))
			{
				downSideObstacles.emplace_back(std::weak_ptr(pawn));
			}
			if (IsCollideWith(LOScheck[RIGHT], pawn->GetShape()))
			{
				rightSideObstacles.emplace_back(std::weak_ptr(pawn));
			}
		}
	}

	// sorting to nearest
	std::ranges::sort(upSideObstacles, [](const std::weak_ptr<BaseObj>& a, const std::weak_ptr<BaseObj>& b)
	{
		const auto aLck = a.lock();
		if (!aLck)
		{
			return false;
		}
		const auto bLck = b.lock();
		if (!bLck)
		{
			return true;
		}
		return aLck->GetPos().y > bLck->GetPos().y;
	});
	std::ranges::sort(leftSideObstacles, [](const std::weak_ptr<BaseObj>& a, const std::weak_ptr<BaseObj>& b)
	{
		const auto aLck = a.lock();
		if (!aLck)
		{
			return false;
		}
		const auto bLck = b.lock();
		if (!bLck)
		{
			return true;
		}
		return aLck->GetPos().x > bLck->GetPos().x;
	});
	std::ranges::sort(downSideObstacles, [](const std::weak_ptr<BaseObj>& a, const std::weak_ptr<BaseObj>& b)
	{
		const auto aLck = a.lock();
		if (!aLck)
		{
			return false;
		}
		const auto bLck = b.lock();
		if (!bLck)
		{
			return true;
		}
		return aLck->GetPos().y < bLck->GetPos().y;
	});
	std::ranges::sort(rightSideObstacles, [](const std::weak_ptr<BaseObj>& a, const std::weak_ptr<BaseObj>& b)
	{
		const auto aLck = a.lock();
		if (!aLck)
		{
			return false;
		}
		const auto bLck = b.lock();
		if (!bLck)
		{
			return true;
		}
		return aLck->GetPos().x < bLck->GetPos().x;
	});

	// priority fire on players
	if (IsPlayerVisible(upSideObstacles, UP))
	{
		_moveBeh->SetDirection(UP);
		Shot();
		return;
	}
	if (IsPlayerVisible(leftSideObstacles, LEFT))
	{
		_moveBeh->SetDirection(LEFT);
		Shot();
		return;
	}
	if (IsPlayerVisible(downSideObstacles, DOWN))
	{
		_moveBeh->SetDirection(DOWN);
		Shot();
		return;
	}
	if (IsPlayerVisible(rightSideObstacles, RIGHT))
	{
		_moveBeh->SetDirection(RIGHT);
		Shot();
		return;
	}

	// fire on obstacle if player not found
	float shootDistance{0.f};
	float bulletOffset{0.f};
	std::shared_ptr<BaseObj> nearestObstacle{nullptr};
	if (dir == UP && !upSideObstacles.empty())
	{
		nearestObstacle = upSideObstacles.front().lock();
		shootDistance = GetY() - nearestObstacle->GetY();
		bulletOffset = GetBulletHeight();
	}
	if (dir == LEFT && !leftSideObstacles.empty())
	{
		nearestObstacle = leftSideObstacles.front().lock();
		shootDistance = GetX() - nearestObstacle->GetX();
		bulletOffset = GetBulletWidth();
	}
	if (dir == DOWN && !downSideObstacles.empty())
	{
		nearestObstacle = downSideObstacles.front().lock();
		shootDistance = nearestObstacle->GetY() - GetY();
		bulletOffset = GetBulletHeight();
	}
	if (dir == RIGHT && !rightSideObstacles.empty())
	{
		nearestObstacle = rightSideObstacles.front().lock();
		shootDistance = nearestObstacle->GetX() - GetX();
		bulletOffset = GetBulletWidth();
	}

	if (nearestObstacle && nearestObstacle.get() && nearestObstacle->GetIsDestructible()
	    && !dynamic_cast<Enemy*>(nearestObstacle.get()))
	{
		if (shootDistance > _bulletDamageAreaRadius + bulletOffset)
		{
			Shot();
		}
	}
}

void Enemy::Move(const float deltaTime)
{
	if (IsTurnCooldownFinish())
	{
		turnDuration = distTurnRate(gen);
		const int randDir = distDirection(gen);
		_moveBeh->SetDirection(static_cast<Direction>(randDir));
		lastTimeTurn = std::chrono::system_clock::now();
	}

	const auto pos = GetPos();
	if (const auto currentDirection = _moveBeh->GetDirection(); currentDirection == LEFT)
	{
		_moveBeh->MoveLeft(deltaTime);
	}
	else if (currentDirection == RIGHT)
	{
		_moveBeh->MoveRight(deltaTime);
	}
	else if (currentDirection == UP)
	{
		_moveBeh->MoveUp(deltaTime);
	}
	else if (currentDirection == DOWN)
	{
		_moveBeh->MoveDown(deltaTime);
	}

	//change dir it cant move
	if (pos == GetPos())
	{
		const int randDir = distDirection(gen);
		_moveBeh->SetDirection(static_cast<Direction>(randDir));
	}

	if (IsReloadFinish())
	{
		MayShoot(_moveBeh->GetDirection());
		lastTimeFire = std::chrono::system_clock::now();
	}
}

bool Enemy::IsTurnCooldownFinish() const
{
	const auto lastTimeTurnSec =
			std::chrono::duration_cast<std::chrono::seconds>(lastTimeTurn.time_since_epoch()).count();
	const auto currentSec =
			std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch())
			.count();

	if (currentSec - lastTimeTurnSec >= turnDuration)
	{
		return true;
	}

	return false;
}
