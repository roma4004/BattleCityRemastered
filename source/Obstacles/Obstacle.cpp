#include <utility>

#include "../../headers/obstacles/Obstacle.h"
#include "../../headers/application/Window.h"
#include "../../headers/components/EventSystem.h"
#include "../../headers/enums/GameMode.h"

Obstacle::Obstacle(ObjRectangle rect, const int color, const int health, std::shared_ptr<Window> window,
                   std::string name, std::shared_ptr<EventSystem> events, const boost::uuids::uuid uuid,
                   const GameMode gameMode, const ObstacleType obstacleType, std::shared_ptr<SDL_Texture> texture,
                   std::shared_ptr<SDL_Renderer> renderer)
	: BaseObj{std::move(rect), color, health, uuid, std::move(name), "Neutral"},
	  _window(std::move(window)),
	  _drawTexture{std::move(texture), std::move(renderer)},
	  _gameMode{gameMode},
	  _obstacleType(obstacleType),
	  _events(std::move(events))
{
	Obstacle::Subscribe();

	if (_gameMode == PlayAsHost)
	{
		_events->EmitEvent<const ObjRectangle, const ObstacleType, const boost::uuids::uuid>(
				"ServerSend_ObstacleSpawn", _rect, _obstacleType, uuid);
	}
}

Obstacle::~Obstacle()
{
	Obstacle::Unsubscribe();
}

void Obstacle::Subscribe()
{
	_events->AddListener("DrawTexture", _nameWithUuid, [this]() { this->_drawTexture.DrawTexture(this); });


	if (_gameMode == PlayAsClient)
	{
		Obstacle::SubscribeAsClient();
	}
}

void Obstacle::SubscribeAsClient()
{
	_events->AddListener<const int>(
			"ClientReceived_" + _nameWithUuid + "Health", _nameWithUuid,
			[this](const int health)
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
	_events->RemoveListener("DrawTexture", _nameWithUuid);

	if (_gameMode == PlayAsClient)
	{
		Obstacle::UnsubscribeAsClient();
	}
}

void Obstacle::UnsubscribeAsClient() const
{
	_events->RemoveListener<const int>("ClientReceived_" + _nameWithUuid + "Health", _nameWithUuid);
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
