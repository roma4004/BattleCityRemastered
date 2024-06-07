#include "../headers/PlayerOne.h"

#include "../headers/EventSystem.h"
#include "../headers/MoveLikeTankBeh.h"

#include <chrono>

PlayerOne::PlayerOne(const Rectangle& rect, const int color, const float speed, const int health, int* windowBuffer,
                     const UPoint windowSize, std::vector<std::shared_ptr<BaseObj>>* allPawns,
                     std::shared_ptr<EventSystem> events)
	: Tank{rect,
	       color,
	       health,
	       windowBuffer,
	       windowSize,
	       allPawns,
	       std::move(events),
	       std::make_shared<MoveLikeTankBeh>(windowSize, speed, this, allPawns)}
{
	BaseObj::SetIsPassable(false);
	BaseObj::SetIsDestructible(true);
	BaseObj::SetIsPenetrable(false);

	// subscribe
	if (_events == nullptr)
	{
		return;
	}

	const std::string name = "PlayerOne";

	_events->AddListener<float>("TickUpdate", name, [this](const float deltaTime) { this->TickUpdate(deltaTime); });

	_events->AddListener("Draw", name, [this]() { this->Draw(); });

	_events->AddListener("W_Pressed", name, [&btn = keyboardButtons]() { btn.up = true; });
	_events->AddListener("W_Released", name, [&btn = keyboardButtons]() { btn.up = false; });
	_events->AddListener("A_Pressed", name, [&btn = keyboardButtons]() { btn.left = true; });
	_events->AddListener("A_Released", name, [&btn = keyboardButtons]() { btn.left = false; });
	_events->AddListener("S_Pressed", name, [&btn = keyboardButtons]() { btn.down = true; });
	_events->AddListener("S_Released", name, [&btn = keyboardButtons]() { btn.down = false; });
	_events->AddListener("D_Pressed", name, [&btn = keyboardButtons]() { btn.right = true; });
	_events->AddListener("D_Released", name, [&btn = keyboardButtons]() { btn.right = false; });
	_events->AddListener("Space_Pressed", name, [&btn = keyboardButtons]() { btn.shot = true; });
	_events->AddListener("Space_Released", name, [this]()
	{
		if (IsReloadFinish())
		{
			keyboardButtons.shot = false;
			this->Shot();
			lastTimeFire = std::chrono::system_clock::now();
		}
	});
}

PlayerOne::~PlayerOne()
{
	// unsubscribe
	if (_events == nullptr)
	{
		return;
	}

	const std::string name = "PlayerOne";

	_events->RemoveListener<float>("TickUpdate", name);

	_events->RemoveListener("Draw", name);

	_events->RemoveListener("W_Pressed", name);
	_events->RemoveListener("W_Released", name);
	_events->RemoveListener("A_Pressed", name);
	_events->RemoveListener("A_Released", name);
	_events->RemoveListener("S_Pressed", name);
	_events->RemoveListener("S_Released", name);
	_events->RemoveListener("D_Pressed", name);
	_events->RemoveListener("D_Released", name);
	_events->RemoveListener("Space_Pressed", name);
	_events->RemoveListener("Space_Released", name);
}
