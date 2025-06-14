#include "../../headers/bonuses/Bonus.h"
#include "../../headers/application/Window.h"
#include "../../headers/components/EventSystem.h"
#include "../../headers/enums/GameMode.h"
#include "../../headers/utils/TimeUtils.h"

Bonus::Bonus(const ObjRectangle& rect, std::shared_ptr<EventSystem> events, const milliseconds duration,
             const milliseconds lifeTime, const int color, std::string name, const buuid uuid, const GameMode gameMode,
             const BonusType bonusType)
	: BaseObj{rect, color, 1, uuid, std::move(name), "Neutral"},
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
		_events->EmitEvent<const FPoint, const BonusType, const buuid&>(
				"ServerSend_BonusSpawn", FPoint{rect.x, rect.y}, _bonusType, uuid);
	}
}

Bonus::~Bonus()
{
	Unsubscribe();

	if (_gameMode == PlayAsHost)
	{
		_events->EmitEvent<const buuid&>("ServerSend_BonusDeSpawn", _uuid);
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
	_events->AddListener<const buuid&>("ClientReceived_BonusDeSpawn", _name, [this](const buuid& uuid)
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
	_events->RemoveListener<const buuid&>("ClientReceived_BonusDeSpawn", _name);
}

void Bonus::Draw(const BaseObj* obj) const { _events->EmitEvent<const BaseObj*>("DrawObj", obj); }

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
