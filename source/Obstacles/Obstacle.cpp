#include "../../headers/obstacles/Obstacle.h"
#include "../../headers/application/Window.h"
#include "../../headers/components/EventSystem.h"
#include "../../headers/enums/GameMode.h"

Obstacle::Obstacle(const ObjRectangle& rect, const int color, const int health, const std::shared_ptr<Window> window,
                   const std::string& name, const std::shared_ptr<EventSystem> events, const int id,
                   const GameMode gameMode)
	: BaseObj{rect, color, health, id, std::move(name), "Neutral"},
	  _window(std::move(window)),
	  _gameMode{gameMode},
	  _events(std::move(events))
{
	Obstacle::Subscribe();
}

Obstacle::~Obstacle()
{
	Obstacle::Unsubscribe();
}

void Obstacle::Subscribe()
{
	_events->AddListener("Draw", _name, [this]() { this->Draw(); });

	if (_gameMode == PlayAsClient)
	{
		Obstacle::SubscribeAsClient();
	}
}

void Obstacle::SubscribeAsClient()
{
	_events->AddListener<const int>("ClientReceived_" + _name + "Health", _name, [this](const int health)
	{
		this->SetHealth(health);
		if (this->GetHealth() < 1)
		{
			this->SetIsAlive(false);
		}
	});
}

void Obstacle::Unsubscribe() const
{
	_events->RemoveListener("Draw", _name);

	if (_gameMode == PlayAsClient)
	{
		Obstacle::UnsubscribeAsClient();
	}
}

void Obstacle::UnsubscribeAsClient() const
{
	_events->RemoveListener<const int>("ClientReceived_" + _name + "Health", _name);
}

void Obstacle::Draw() const
{
	if (!GetIsAlive())
	{
		return;
	}

	int startY = static_cast<int>(GetY());
	const int startX = static_cast<int>(GetX());
	const size_t windowWidth = _window->size.x;
	const int height = static_cast<int>(GetHeight());
	const int width = static_cast<int>(GetWidth());
	const int color = GetColor();
	const int maxY = startY + height;
	const int maxX = startX + width;

	for (; startY < maxY; ++startY)
	{
		for (int x = startX; x < maxX; ++x)
		{
			const size_t offset = startY * windowWidth + startX;
			const int rowWidth = maxX - startX;
			std::ranges::fill_n(_window->buffer.get() + offset, rowWidth, color);
		}
	}
}
