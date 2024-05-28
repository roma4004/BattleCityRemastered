#include "../headers/PlayerTwo.h"

PlayerTwo::PlayerTwo(const FPoint& pos, const float width, const float height, const int color, const float speed,
					 const int health, int* windowBuffer, size_t windowWidth, size_t windowHeight,
					 std::vector<std::shared_ptr<BaseObj>>* allPawns, std::shared_ptr<EventSystem> events)
	: Pawn(pos, width, height, color, speed, health, windowBuffer, windowWidth, windowHeight, allPawns, std::move(events))
{
	BaseObj::SetIsPassable(false);
	BaseObj::SetIsDestructible(true);
	BaseObj::SetIsPenetrable(false);

	// subscribe
	if (_events == nullptr)
	{
		return;
	}

	const std::string listenerName = "PlayerTwo";

	_events->AddListener<float>("TickUpdate", listenerName, [this](const float deltaTime) { this->TickUpdate(deltaTime); });

	_events->AddListener("Draw", listenerName, [this]() { this->Draw(); });
}

PlayerTwo::~PlayerTwo()
{
	// unsubscribe
	if (_events == nullptr)
	{
		return;
	}

	const std::string listenerName = "PlayerTwo";

	_events->RemoveListener<float>("TickUpdate", listenerName);

	_events->RemoveListener("Draw", listenerName);
}

void PlayerTwo::KeyboardEvensHandlers(const Uint32 eventType, const SDL_Keycode key)
{
	if (eventType == SDL_KEYDOWN)
	{
		if (key == SDLK_LEFT)
		{
			keyboardButtons.a = true;
		}
		else if (key == SDLK_RIGHT)
		{
			keyboardButtons.d = true;
		}
		else if (key == SDLK_DOWN)
		{
			keyboardButtons.s = true;
		}
		else if (key == SDLK_UP)
		{
			keyboardButtons.w = true;
		}
	}
	else if (eventType == SDL_KEYUP)
	{
		if (key == SDLK_LEFT)
		{
			keyboardButtons.a = false;
		}
		else if (key == SDLK_RIGHT)
		{
			keyboardButtons.d = false;
		}
		else if (key == SDLK_DOWN)
		{
			keyboardButtons.s = false;
		}
		else if (key == SDLK_UP)
		{
			keyboardButtons.w = false;
		}
		else if (key == SDLK_RCTRL)
		{
			keyboardButtons.shot = true;
		}
	}
}
