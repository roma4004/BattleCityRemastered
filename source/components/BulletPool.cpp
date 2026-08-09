#include "components/BulletPool.h"
#include "application/GameConfig.h"
#include "components/EventSystem.h"
#include "components/events/CoreLifecycleEvents.h"
#include "components/events/GameModeEvents.h"
#include "components/events/ObjectLifecycleEvents.h"
#include "entities/pawns/Bullet.h"
#include "entities/pawns/PawnProperty.h"
#include "enums/GameMode.h"

BulletPool::BulletPool(const std::shared_ptr<EventSystem>& events, std::vector<std::shared_ptr<BaseObj>>* allObjects,
					   GameConfig& gameConfig)
	: _name{"BulletPool"}
	, _events{events}
	, _allObjects{allObjects}
	, _gameMode{GameMode::Demo}
	, _gameConfig{gameConfig}
{
	// Pre-generate 20 default bullets
	for (size_t i = 0u; i < 20u; ++i)
	{
		_bullets.push(CreateNewBullet());
	}

	Subscribe();
}

std::string BulletPool::GetCurrentTimeString()
{
	const auto now = std::chrono::system_clock::now();
	const auto nowTime = std::chrono::system_clock::to_time_t(now);
	const auto ms = std::chrono::duration_cast<milliseconds>(now.time_since_epoch()) % 1000;

	std::tm timeInfo;
	localtime_s(&timeInfo, &nowTime);

	std::stringstream ss;
	ss << std::put_time(&timeInfo, "%H:%M:%S") << '.' << std::setfill('0') << std::setw(3) << ms.count();

	return ss.str();
}

void BulletPool::Subscribe()
{
	_subs.push_back(_events->AddListener(_name, [this](const GameResetEvent&) { Clear(); }));

	_subs.push_back(_events->AddListener(_name, [this](const GameModeChangedToEvent& event)
	{
		_gameMode = event.mode;
	}));
}

std::shared_ptr<Bullet> BulletPool::CreateNewBullet()
{
	PawnProperty pawnProperty{.baseObjProperty = {},
							  .allObjects = _allObjects,
							  .events = _events,
							  .gameMode = _gameMode};

	return {new Bullet{std::move(pawnProperty), _gameConfig}, [this](Bullet* b) { ReturnBullet(b); }};
}

std::shared_ptr<BaseObj> BulletPool::SpawnBullet()
{
	std::scoped_lock lock(_bulletsMutex);

	if (_bullets.empty())
	{
		return CreateNewBullet();
	}

	std::shared_ptr<BaseObj> bulletAsBase = _bullets.front();
	_bullets.pop();

	if (const auto* bullet = dynamic_cast<Bullet*>(bulletAsBase.get());
		bulletAsBase != nullptr && bullet != nullptr)
	{
		// std::cout << "[" << GetCurrentTimeString() << "] "
		// 		<< "[" << (_gameMode == PlayAsHost ? "SERVER" : "CLIENT") << "] "
		// 		<< "Bullet REUSED and Bullet pool size =" << _bullets.size()
		// 		<< ", Author=" << author
		// 		<< ", Direction=" << static_cast<int>(dir)
		// 		<< ", Fraction=" << fraction
		// 		<< ", UUID=" << bullet->GetUuid()
		// 		<< '\n';
	}

	return bulletAsBase;
}

void BulletPool::ReturnBullet(BaseObj* bullet)
{
	if (_isClearing)
	{
		delete bullet;

		return;
	}

	std::scoped_lock lock(_bulletsMutex);
	if (const auto* bulletCast = dynamic_cast<Bullet*>(bullet); bulletCast != nullptr)
	{
		// std::cout << "[" << GetCurrentTimeString() << "] "
		// 		<< "[" << (_gameMode == PlayAsHost ? "SERVER" : "CLIENT") << "] "
		// 		<< "Bullet RETURNED to pool and Bullet pool size =" << _bullets.size()
		// 		<< ", Author=" << bulletCast->GetAuthor()
		// 		<< ", UUID=" << bulletCast->GetUuid()
		// 		<< '\n';

		bulletCast->Disable();
		_bullets.emplace(std::shared_ptr<BaseObj>(bullet, [this](BaseObj* b)
		{
			ReturnBullet(b);
		}));

		_events->EmitEvent(ServerSendDisposeEvent{.uuid = bulletCast->GetUuid()});
	}
}

void BulletPool::Clear()
{
	std::scoped_lock lock(_bulletsMutex);
	_isClearing = true;

	// std::cout << "[" << GetCurrentTimeString() << "] "
	// 		<< "[" << (_gameMode == PlayAsHost ? "SERVER" : "CLIENT") << "] "
	// 		<< "Bullet pool CLEARED, bullets in pool: " << _bullets.size()
	// 		<< '\n';

	while (!_bullets.empty())
	{
		_bullets.pop();
	}

	_isClearing = false;
}
