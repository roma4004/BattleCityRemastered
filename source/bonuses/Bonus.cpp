#include "../../headers/bonuses/Bonus.h"
#include "../../headers/utils/TimeUtils.h"

Bonus::Bonus(const ObjRectangle& rect, std::shared_ptr<Window> window, std::shared_ptr<EventSystem> events,
             const std::chrono::milliseconds duration, const std::chrono::milliseconds lifeTime, const int color,
             std::string name, const int id, const GameMode gameMode)
	: BaseObj{{.x = rect.x, .y = rect.y, .w = rect.w - 1, .h = rect.h - 1}, color, 1, id, std::move(name), "Neutral"},
	  _window{std::move(window)},
	  _creationTime{std::chrono::system_clock::now()},
	  _gameMode{gameMode},
	  _duration{duration},
	  _lifetime{lifeTime},
	  _events{std::move(events)}
{
	BaseObj::SetIsPassable(false);
	BaseObj::SetIsDestructible(true);
	BaseObj::SetIsPenetrable(false);

	Subscribe();
}

Bonus::~Bonus()
{
	Unsubscribe();

	if (_gameMode == PlayAsHost)
	{
		_events->EmitEvent<const int>("ServerSend_BonusDeSpawn", _id);//TODO: move to pick up moment in tank move beh
	}
}

void Bonus::Subscribe()
{
	_events->AddListener("Draw", _name + std::to_string(_id), [this]() { this->Draw(); });

	_gameMode == PlayAsClient ? SubscribeAsClient() : SubscribeAsHost();
}

void Bonus::SubscribeAsHost()
{
	_events->AddListener<const float>("TickUpdate", _name + std::to_string(_id), [this](const float deltaTime)
	{
		this->TickUpdate(deltaTime);
	});
}

void Bonus::SubscribeAsClient()
{
	_events->AddListener<const int>("ClientReceived_BonusDeSpawn", _name, [this](const int id)
	{
		if (id == this->_id)
		{
			this->SetIsAlive(false);
		}
	});
}

void Bonus::Unsubscribe() const
{
	_gameMode == PlayAsClient ? UnsubscribeAsClient() : UnsubscribeAsHost();

	_events->RemoveListener("Draw", _name + std::to_string(_id));
}

void Bonus::UnsubscribeAsHost() const
{
	_events->RemoveListener<const float>("TickUpdate", _name + std::to_string(_id));
}

void Bonus::UnsubscribeAsClient() const
{
	_events->RemoveListener<const int>("ClientReceived_BonusDeSpawn", _name);
}

void Bonus::Draw() const
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

	for (const int maxY = startY + height; startY < maxY; ++startY)
	{
		int x = startX;
		for (const int maxX = x + width; x < maxX; ++x)
		{
			const size_t offset = startY * windowWidth + startX;
			const int rowWidth = maxX - startX;
			std::ranges::fill_n(_window->buffer.get() + offset, rowWidth, color);
		}
	}
}

void Bonus::TickUpdate(float /*deltaTime*/)
{
	if (TimeUtils::IsCooldownFinish(_creationTime, _lifetime))
	{
		SetIsAlive(false);
	}
}
