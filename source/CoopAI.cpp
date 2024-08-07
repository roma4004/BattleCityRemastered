#include "../headers/CoopAI.h"
#include "../headers/Enemy.h"
#include "../headers/EventSystem.h"
#include "../headers/MoveLikeAIBeh.h"
#include "../headers/PlayerOne.h"

#include <algorithm>
#include <chrono>

CoopAI::CoopAI(const Rectangle& rect, const int color, const int health, int* windowBuffer, const UPoint windowSize,
               Direction direction, float speed, std::vector<std::shared_ptr<BaseObj>>* allObjects,
               std::shared_ptr<EventSystem> events, std::string name, std::string fraction,
               std::shared_ptr<BulletPool> bulletPool)
	: Tank{rect,
	       color,
	       health,
	       windowBuffer,
	       windowSize,
	       direction,
	       speed,
	       allObjects,
	       std::move(events),
	       std::make_shared<MoveLikeAIBeh>(this, allObjects),
	       std::move(bulletPool),
	       std::move(name),
	       std::move(fraction)},
	  distDirection(0, 3), distTurnRate(1, 5)
{
	BaseObj::SetIsPassable(false);
	BaseObj::SetIsDestructible(true);
	BaseObj::SetIsPenetrable(false);

	std::random_device rd;
	gen = std::mt19937(std::chrono::high_resolution_clock::now().time_since_epoch().count() + rd());

	Subscribe();

	_events->EmitEvent(_name + "_Spawn");
}

CoopAI::~CoopAI()
{
	Unsubscribe();

	_events->EmitEvent(_name + "_Died");
}

void CoopAI::Subscribe()
{
	if (_events == nullptr)
	{
		return;
	}

	_events->AddListener<const float>("TickUpdate", _name, [this](const float deltaTime)
	{
		this->TickUpdate(deltaTime);
	});

	_events->AddListener("Draw", _name, [this]() { this->Draw(); });

	_events->AddListener("DrawHealthBar", _name, [this]() { this->DrawHealthBar(); });
}

void CoopAI::Unsubscribe() const
{
	if (_events == nullptr)
	{
		return;
	}

	_events->RemoveListener<const float>("TickUpdate", _name);

	_events->RemoveListener("Draw", _name);

	_events->RemoveListener("DrawHealthBar", _name);
}

bool CoopAI::IsCollideWith(const Rectangle& r1, const Rectangle& r2)
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

bool CoopAI::IsEnemyVisible(const std::vector<std::weak_ptr<BaseObj>>& obstacles)
{
	if (!obstacles.empty())
	{
		std::shared_ptr<BaseObj> isEnemySeen = obstacles.front().lock();
		if (dynamic_cast<Enemy*>(isEnemySeen.get()))
		{
			return true;
		}
	}

	return false;
}

void CoopAI::MayShoot(Direction dir)
{
	const FPoint windowSize = {static_cast<float>(_windowSize.x), static_cast<float>(_windowSize.y)};
	const float tankHalfWidth = GetWidth() / 2.f;
	const float tankHalfHeight = GetHeight() / 2.f;
	const float bulletHalfWidth = GetBulletWidth() / 2.f;
	const float bulletHalfHeight = GetBulletHeight() / 2.f;
	std::vector<Rectangle> LOScheck{
			/*up, left, down, right*/
			{_shape.x + tankHalfWidth - bulletHalfWidth, 0, GetBulletWidth(), _shape.y},
			{0, _shape.y + tankHalfHeight - bulletHalfHeight, _shape.x, GetBulletHeight()},
			{_shape.x + tankHalfWidth - bulletHalfWidth, _shape.y + GetHeight(), GetBulletWidth(), windowSize.y},
			{_shape.x + GetWidth(), _shape.y + tankHalfHeight - bulletHalfHeight, windowSize.x, GetBulletHeight()}};

	// parse all seen in LOS (line of sight) obj
	std::vector<std::weak_ptr<BaseObj>> upSideObstacles{};
	std::vector<std::weak_ptr<BaseObj>> leftSideObstacles{};
	std::vector<std::weak_ptr<BaseObj>> downSideObstacles{};
	std::vector<std::weak_ptr<BaseObj>> rightSideObstacles{};
	for (std::shared_ptr<BaseObj>& object: *_allObjects)
	{
		if (this == object.get())
		{
			continue;
		}

		if (!object->GetIsPassable() && !object->GetIsPenetrable())
		{
			if (IsCollideWith(LOScheck[UP], object->GetShape()))
			{
				upSideObstacles.emplace_back(std::weak_ptr(object));
			}
			if (IsCollideWith(LOScheck[LEFT], object->GetShape()))
			{
				leftSideObstacles.emplace_back(std::weak_ptr(object));
			}
			if (IsCollideWith(LOScheck[DOWN], object->GetShape()))
			{
				downSideObstacles.emplace_back(std::weak_ptr(object));
			}
			if (IsCollideWith(LOScheck[RIGHT], object->GetShape()))
			{
				rightSideObstacles.emplace_back(std::weak_ptr(object));
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
	if (IsEnemyVisible(upSideObstacles))
	{
		SetDirection(UP);
		Shot();
		return;
	}
	if (IsEnemyVisible(leftSideObstacles))
	{
		SetDirection(LEFT);
		Shot();
		return;
	}
	if (IsEnemyVisible(downSideObstacles))
	{
		SetDirection(DOWN);
		Shot();
		return;
	}
	if (IsEnemyVisible(rightSideObstacles))
	{
		SetDirection(RIGHT);
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
	    && !dynamic_cast<PlayerOne*>(nearestObstacle.get()) && !dynamic_cast<CoopAI*>(nearestObstacle.get()))
	{
		if (shootDistance > _bulletDamageAreaRadius + bulletOffset)
		{
			Shot();
		}
	}
}

void CoopAI::TickUpdate(const float deltaTime)
{
	// change dir when random time span left
	if (IsTurnCooldownFinish())
	{
		turnDuration = distTurnRate(gen);
		const int randDir = distDirection(gen);
		SetDirection(static_cast<Direction>(randDir));
		lastTimeTurn = std::chrono::system_clock::now();
	}

	// move
	const auto pos = GetPos();
	_moveBeh->Move(deltaTime);

	// change dir it cant move
	if (pos == GetPos())
	{
		const int randDir = distDirection(gen);
		SetDirection(static_cast<Direction>(randDir));
	}

	// shot
	if (IsReloadFinish())
	{
		MayShoot(GetDirection());
		lastTimeFire = std::chrono::system_clock::now();
	}
}

bool CoopAI::IsTurnCooldownFinish() const
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

void CoopAI::SendDamageStatistics(const std::string& author, const std::string& fraction)
{
	_events->EmitEvent<const std::string&, const std::string&>("CoopAIHit", author, fraction);

	if (GetHealth() < 1)
	{
		_events->EmitEvent<const std::string&, const std::string&>("CoopAIDied", author, fraction);
	}
}
