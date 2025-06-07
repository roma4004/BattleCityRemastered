#include "../../headers/bonuses/Bonus.h"
#include "../../headers/application/Window.h"
#include "../../headers/components/EventSystem.h"
#include "../../headers/enums/GameMode.h"
#include "../../headers/utils/TimeUtils.h"

#include <boost/uuid/uuid_io.hpp>

Bonus::Bonus(const ObjRectangle& rect, std::shared_ptr<Window> window, std::shared_ptr<EventSystem> events,
             const std::chrono::milliseconds duration, const std::chrono::milliseconds lifeTime, const int color,
             std::string name, const boost::uuids::uuid uuid, const GameMode gameMode, const BonusType bonusType)
	: BaseObj{rect, color, 1, uuid, std::move(name), "Neutral"},
	  _window{std::move(window)},
	  _creationTime{std::chrono::system_clock::now()},
	  _gameMode{gameMode},
	  _bonusType{bonusType},
	  _effectDuration{duration},
	  _lifetime{lifeTime},
	  _events{std::move(events)}
{
	BaseObj::SetIsPassable(false);
	BaseObj::SetIsDestructible(true);
	BaseObj::SetIsPenetrable(false);

	Subscribe();

	if (_gameMode == PlayAsHost)
	{
		_events->EmitEvent<const FPoint, const BonusType, const boost::uuids::uuid>(
				"ServerSend_BonusSpawn", FPoint{rect.x, rect.y}, _bonusType, uuid);
	}
}

Bonus::~Bonus()
{
	Unsubscribe();

	if (_gameMode == PlayAsHost)
	{
		_events->EmitEvent<const boost::uuids::uuid&>("ServerSend_BonusDeSpawn", _uuid);
		//TODO: move to pick up moment in tank move beh
	}
}

void Bonus::Subscribe()
{
	_events->AddListener("Draw", _nameWithUuid, [this]() { this->Draw(this); });

	_gameMode == PlayAsClient ? SubscribeAsClient() : SubscribeAsHost();
}

void Bonus::SubscribeAsHost()
{
	_events->AddListener<const float>("TickUpdate", _nameWithUuid, [this](const float deltaTime)
	{
		this->TickUpdate(deltaTime);
	});
}

void Bonus::SubscribeAsClient()
{
	_events->AddListener<const boost::uuids::uuid&>(
			"ClientReceived_BonusDeSpawn", _name,
			[this](const boost::uuids::uuid& uuid)
			{
				if (uuid != this->_uuid)
				{
					return;
				}

				this->SetIsAlive(false);
			});
}

void Bonus::Unsubscribe() const
{
	_gameMode == PlayAsClient ? UnsubscribeAsClient() : UnsubscribeAsHost();

	_events->RemoveListener("Draw", _nameWithUuid);
}

void Bonus::UnsubscribeAsHost() const
{
	_events->RemoveListener<const float>("TickUpdate", _nameWithUuid);
}

void Bonus::UnsubscribeAsClient() const
{
	_events->RemoveListener<const boost::uuids::uuid&>("ClientReceived_BonusDeSpawn", _name);
}

void Bonus::Draw(const BaseObj* /*obj*/) const
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

void Bonus::SendDamageStatistics(const std::string& author, const std::string& fraction)
{
	_events->EmitEvent<const std::string&, const std::string&>(_name, author, fraction);
}

void Bonus::PickUpBonus(const std::string& author, const std::string& fraction)
{
	_events->EmitEvent<const std::string&, const std::string&>(_name, author, fraction);
}
