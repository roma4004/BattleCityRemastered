#include "entities/bonuses/Bonus.h"
#include "Point.h"
#include "components/EventSystem.h"
#include "enums/Direction.h"
#include "enums/GameMode.h"
#include "utils/TimeUtils.h"

Bonus::Bonus(const ObjRectangle& rect, const std::shared_ptr<EventSystem>& events, const milliseconds lifeTime,
			 const unsigned int color, std::string name, const buuid uuid, const GameMode gameMode,
			 const BonusType bonusType)
	: BaseObj{rect, color, 1, uuid, std::move(name), "Neutral"}
	, _creationTime{std::chrono::system_clock::now()}
	, _events{events}
	, _lifetime{lifeTime}
	, _gameMode{gameMode}
	, _bonusType{bonusType}
{
	BaseObj::SetIsPassable(false);
	BaseObj::SetIsDestructible(true);
	BaseObj::SetIsPenetrable(false);

	Subscribe();

	if (_gameMode == GameMode::PlayAsHost)
	{
		_events->EmitEvent("ServerSend_BonusSpawn", FPoint{.x = rect.x, .y = rect.y}, _bonusType, uuid);
	}
}

Bonus::~Bonus()
{
	Unsubscribe();

	if (_gameMode == GameMode::PlayAsHost)
	{
		_events->EmitEvent("ServerSend_BonusDeSpawn", _uuid);
	}
}

void Bonus::Subscribe()
{
	_events->AddListener("Draw", _nameWithUuid, [this]() { this->Draw(); });

	_gameMode == GameMode::PlayAsClient ? SubscribeAsClient() : SubscribeAsHost();
}

void Bonus::SubscribeAsHost()
{
	_events->AddListener("TickUpdate", _nameWithUuid, [this](const double deltaTime)
	{
		this->TickUpdate(deltaTime);
	});
}

void Bonus::SubscribeAsClient()
{
	_events->AddListener("ClientReceived_BonusDeSpawn", _name, [this](const buuid& uuid)
	{
		if (uuid != this->_uuid)
		{
			return;
		}

		this->SetIsAlive(false);
	});
}

void Bonus::Unsubscribe() const { _events->RemoveAllListeners(_nameWithUuid); }

void Bonus::Draw() const { _events->EmitEvent("DrawObj", _rect, Direction::UP, _name, _color); }

void Bonus::TickUpdate(double /*deltaTime*/)
{
	if (TimeUtils::IsCooldownFinish(_creationTime, _lifetime)) //TODO: extract to BonusEffectManager
	{
		SetIsAlive(false);
	}
}

void Bonus::SendDamageStatistics(const std::string& author, const std::string& fraction)
{
	_events->EmitEvent(_name, author, fraction);
}

void Bonus::PickUpBonus(const std::string& author, const std::string& fraction)
{
	_events->EmitEvent(_name, author, fraction);
}
