#include "../headers/PlayerTwo.h"

#include "../headers/Bullet.h"
#include "../headers/EventSystem.h"
#include "../headers/MoveLikeTankBeh.h"

#include <chrono>

PlayerTwo::PlayerTwo(const Rectangle& rect, const int color, const float speed, const int health, int* windowBuffer,
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

	const std::string name = "PlayerTwo";

	_events->AddListener<float>("TickUpdate", name, [this](const float deltaTime) { this->TickUpdate(deltaTime); });

	_events->AddListener("Draw", name, [this]() { this->Draw(); });

	_events->AddListener("ArrowUp_Pressed", name, [&btn = keyboardButtons]() { btn.up = true; });
	_events->AddListener("ArrowUp_Released", name, [&btn = keyboardButtons]() { btn.up = false; });
	_events->AddListener("ArrowLeft_Pressed", name, [&btn = keyboardButtons]() { btn.left = true; });
	_events->AddListener("ArrowLeft_Released", name, [&btn = keyboardButtons]() { btn.left = false; });
	_events->AddListener("ArrowDown_Pressed", name, [&btn = keyboardButtons]() { btn.down = true; });
	_events->AddListener("ArrowDown_Released", name, [&btn = keyboardButtons]() { btn.down = false; });
	_events->AddListener("ArrowRight_Pressed", name, [&btn = keyboardButtons]() { btn.right = true; });
	_events->AddListener("ArrowRight_Released", name, [&btn = keyboardButtons]() { btn.right = false; });
	_events->AddListener("RCTRL_Pressed", name, [&btn = keyboardButtons]() { btn.shot = true; });
	_events->AddListener("RCTRL_Released", name, [this]()
	{
		if (IsReloadFinish())
		{
			keyboardButtons.shot = false;
			this->Shot();
			lastTimeFire = std::chrono::system_clock::now();
		}
	});

	_events->EmitEvent("Statistics_P2_Respawn");
}

PlayerTwo::~PlayerTwo()
{
	// unsubscribe
	if (_events == nullptr)
	{
		return;
	}

	const std::string name = "PlayerTwo";

	_events->RemoveListener<float>("TickUpdate", name);

	_events->RemoveListener("Draw", name);

	_events->RemoveListener("ArrowUp_Pressed", name);
	_events->RemoveListener("ArrowUp_Released", name);
	_events->RemoveListener("ArrowLeft_Pressed", name);
	_events->RemoveListener("ArrowLeft_Released", name);
	_events->RemoveListener("ArrowDown_Pressed", name);
	_events->RemoveListener("ArrowDown_Released", name);
	_events->RemoveListener("ArrowRight_Pressed", name);
	_events->RemoveListener("ArrowRight_Released", name);
	_events->RemoveListener("RCTRL_Pressed", name);
	_events->RemoveListener("RCTRL_Released", name);

	_events->EmitEvent("Statistics_P2_Died");
}
