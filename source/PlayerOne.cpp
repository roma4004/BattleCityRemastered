#include "../headers/PlayerOne.h"

#include "../headers/EventSystem.h"
#include "../headers/MoveLikeTankBeh.h"

#include <chrono>

PlayerOne::PlayerOne(const Rectangle& rect, const int color, const float speed, const int health, int* windowBuffer,
                     const UPoint windowSize, std::vector<std::shared_ptr<BaseObj>>* allPawns,
                     std::shared_ptr<EventSystem> events, std::string name,
                     std::unique_ptr<IInputProvider>& inputProvider, std::shared_ptr<BulletPool> bulletPool)
	: Tank{rect,
	       color,
	       health,
	       windowBuffer,
	       windowSize,
	       allPawns,
	       std::move(events),
	       std::make_shared<MoveLikeTankBeh>(UP, windowSize, speed, this, allPawns),
	       std::move(bulletPool)},
	  _name{std::move(name)},
	  _inputProvider{std::move(inputProvider)}
{
	BaseObj::SetIsPassable(false);
	BaseObj::SetIsDestructible(true);
	BaseObj::SetIsPenetrable(false);

	Subscribe();
}

PlayerOne::~PlayerOne()
{
	Unsubscribe();
}

void PlayerOne::Subscribe()
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
}

void PlayerOne::Unsubscribe() const
{
	if (_events == nullptr)
	{
		return;
	}

	_events->RemoveListener<float>("TickUpdate", _name);

	_events->RemoveListener("Draw", _name);

	_events->EmitEvent("P1_Died");
}

void PlayerOne::Move(const float deltaTime)
{
	if (_inputProvider->playerKeys.left)
	{
		_moveBeh->SetDirection(LEFT);
		_moveBeh->MoveLeft(deltaTime);
	}
	else if (_inputProvider->playerKeys.right)
	{
		_moveBeh->SetDirection(RIGHT);
		_moveBeh->MoveRight(deltaTime);
	}
	else if (_inputProvider->playerKeys.up)
	{
		_moveBeh->SetDirection(UP);
		_moveBeh->MoveUp(deltaTime);
	}
	else if (_inputProvider->playerKeys.down)
	{
		_moveBeh->SetDirection(DOWN);
		_moveBeh->MoveDown(deltaTime);
	}
}
