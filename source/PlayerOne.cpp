#include "../headers/PlayerOne.h"

PlayerOne::PlayerOne(const Rectangle& rect, const int color, const float speed, const int health, int* windowBuffer,
					 const UPoint windowSize, std::vector<std::shared_ptr<BaseObj>>* allPawns,
					 std::shared_ptr<EventSystem> events)
	: Pawn{rect, color, speed, health, windowBuffer, windowSize, allPawns, std::move(events)}
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

	_events->AddListener("W_Pressed", name, [&btn = keyboardButtons]() { btn.w = true; });
	_events->AddListener("W_Released", name, [&btn = keyboardButtons]() { btn.w = false; });
	_events->AddListener("A_Pressed", name, [&btn = keyboardButtons]() { btn.a = true; });
	_events->AddListener("A_Released", name, [&btn = keyboardButtons]() { btn.a = false; });
	_events->AddListener("S_Pressed", name, [&btn = keyboardButtons]() { btn.s = true; });
	_events->AddListener("S_Released", name, [&btn = keyboardButtons]() { btn.s = false; });
	_events->AddListener("D_Pressed", name, [&btn = keyboardButtons]() { btn.d = true; });
	_events->AddListener("D_Released", name, [&btn = keyboardButtons]() { btn.d = false; });
	_events->AddListener("Space_Pressed", name, [&btn = keyboardButtons]() { btn.shot = true; });
	_events->AddListener("Space_Released", name, [&btn = keyboardButtons]() { btn.shot = false; });
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
