#include "../headers/PlayerTwo.h"

#include "../headers/EventSystem.h"
#include "../headers/MoveLikeTankBeh.h"

#include <chrono>

PlayerTwo::PlayerTwo(const Rectangle& rect, const int color, const int health, int* windowBuffer,
                     const UPoint windowSize, Direction direction, float speed,
                     std::vector<std::shared_ptr<BaseObj>>* allObjects, std::shared_ptr<EventSystem> events,
                     std::string name, std::string fraction, std::unique_ptr<IInputProvider>& inputProvider,
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
	       std::make_shared<MoveLikeTankBeh>(this, allObjects),
	       std::move(bulletPool),
	       std::move(name),
	       std::move(fraction)},
	  _inputProvider{std::move(inputProvider)}
{
	BaseObj::SetIsPassable(false);
	BaseObj::SetIsDestructible(true);
	BaseObj::SetIsPenetrable(false);

	Subscribe();

	_events->EmitEvent(_name + "_Spawn");
}


PlayerTwo::~PlayerTwo()
{
	Unsubscribe();

	_events->EmitEvent(_name + "_Died");
}

void PlayerTwo::Subscribe()
{
	if (_events == nullptr)
	{
		return;
	}

	_events->AddListener<float>("TickUpdate", _name, [this](const float deltaTime)
	{
		// move
		this->TickUpdate(deltaTime);

		// shot
		if (_inputProvider->playerKeys.shot && IsReloadFinish())
		{
			this->Shot();
			lastTimeFire = std::chrono::system_clock::now();
		}
	});

	_events->AddListener("Draw", _name, [this]() { this->Draw(); });

	_events->AddListener("DrawHealthBar", _name, [this]() { this->DrawHealthBar(); });
}

void PlayerTwo::Unsubscribe() const
{
	if (_events == nullptr)
	{
		return;
	}

	_events->RemoveListener<float>("TickUpdate", _name);

	_events->RemoveListener("Draw", _name);

	_events->RemoveListener("DrawHealthBar", _name);
}

void PlayerTwo::TickUpdate(const float deltaTime)
{
	if (_inputProvider->playerKeys.left)
	{
		SetDirection(LEFT);
		_moveBeh->Move(deltaTime);
	}
	else if (_inputProvider->playerKeys.right)
	{
		SetDirection(RIGHT);
		_moveBeh->Move(deltaTime);
	}
	else if (_inputProvider->playerKeys.up)
	{
		SetDirection(UP);
		_moveBeh->Move(deltaTime);
	}
	else if (_inputProvider->playerKeys.down)
	{
		SetDirection(DOWN);
		_moveBeh->Move(deltaTime);
	}
}

void PlayerTwo::SendDamageStatistics(const std::string& author, const std::string& fraction)
{
	std::string authorAndFractionTag = author + fraction;
	_events->EmitEvent<std::string>("PlayerTwoHit", authorAndFractionTag);

	if (GetHealth() < 1)
	{
		std::string authorAndFractionDieTag = author + fraction;
		_events->EmitEvent<std::string>("PlayerTwoDied", authorAndFractionDieTag);
	}
}
