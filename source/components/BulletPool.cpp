#include "../../headers/components/BulletPool.h"
#include "../../headers/components/EventSystem.h"
#include "../../headers/enums/Direction.h"
#include "../../headers/enums/GameMode.h"
#include "../../headers/pawns/Bullet.h"
#include "../../headers/pawns/PawnProperty.h"

#include <chrono>
#include <iomanip>
#include <iostream>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

BulletPool::BulletPool(std::shared_ptr<EventSystem> events, std::vector<std::shared_ptr<BaseObj>>* allObjects,
                       std::shared_ptr<Window> window, const GameMode gameMode)
	: _events{std::move(events)},
	  _name{"BulletPool"},
	  _gameMode{gameMode},
	  _allObjects{allObjects},
	  _window{std::move(window)}
{
	// Pre-generate 20 default bullets
	// for (int i = 0; i < 20; ++i)
	// {
	// 	constexpr ObjRectangle rect{0, 0, 2, 2};
	// 	auto bullet = CreateNewBullet(
	// 			rect, 1, 0.0, 0xFFFFFF, 1, Direction::UP, 1.0f, "Default", "Default", 1);
	// 	_bullets.push(bullet);
	// }

	Subscribe();
}

BulletPool::~BulletPool()
{
	Unsubscribe();
}

std::string BulletPool::GetCurrentTimeString()
{
	const auto now = std::chrono::system_clock::now();
	const auto nowTime = std::chrono::system_clock::to_time_t(now);
	const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

	std::tm timeInfo;
	localtime_s(&timeInfo, &nowTime);

	std::stringstream ss;
	ss << std::put_time(&timeInfo, "%H:%M:%S") << '.'
			<< std::setfill('0') << std::setw(3) << ms.count();

	return ss.str();
}

void BulletPool::Subscribe()
{
	_events->AddListener("Reset", _name, [this]() { Clear(); });

	_events->AddListener<const GameMode>("GameModeChangedTo", _name, [this](const GameMode newGameMode)
	{
		_gameMode = newGameMode;
	});
}

void BulletPool::Unsubscribe() const
{
	_events->RemoveListener("Reset", _name);

	_events->RemoveListener<const GameMode>("GameModeChangedTo", _name);
}

std::shared_ptr<Bullet> BulletPool::CreateNewBullet(ObjRectangle rect, const int damage, const double aoeRadius,
                                                    const int color, const int health, const Direction dir,
                                                    const float speed, std::string author, std::string fraction,
                                                    const int tier)
{
	static boost::uuids::random_generator uuidGenerator;
	boost::uuids::uuid bulletUuid = uuidGenerator();
	std::string uuidStr = boost::uuids::to_string(bulletUuid);
	std::string name{"Bullet"};

	// std::cout << "[" << GetCurrentTimeString() << "] "
	// 		<< "[" << (_gameMode == PlayAsHost ? "SERVER" : "CLIENT") << "] "
	// 		<< "Bullet CREATED and Bullet pool size =" << _bullets.size()
	// 		<< ", Author=" << author
	// 		<< ", Direction=" << static_cast<int>(dir)
	// 		<< ", Fraction=" << fraction
	// 		<< ", UUID=" << bulletUuid
	// 		<< std::endl;

	BaseObjProperty baseObjProperty{
			std::move(rect), color, health, true, bulletUuid, std::move(name), std::move(fraction)};
	PawnProperty pawnProperty{
			std::move(baseObjProperty), _window, dir, speed, _allObjects, _events, tier, _gameMode};
	return std::shared_ptr<Bullet>(
			new Bullet{std::move(pawnProperty), damage, aoeRadius, std::move(author), bulletUuid, uuidStr},
			[this](Bullet* b)
			{
				ReturnBullet(b);
			});
}

std::shared_ptr<BaseObj> BulletPool::SpawnBullet(const ObjRectangle rect, const int damage, const double aoeRadius,
                                                 const int color, const int health, const Direction dir,
                                                 const float speed, std::string author, std::string fraction,
                                                 const int tier)
{
	std::lock_guard<std::mutex> lock(_bulletsMutex);

	if (_bullets.empty())
	{
		std::shared_ptr<BaseObj> bullet = CreateNewBullet(
				rect, damage, aoeRadius, color, health, dir, speed, std::move(author), std::move(fraction), tier);

		return bullet;
	}

	std::shared_ptr<BaseObj> bulletAsBase = _bullets.front();
	_bullets.pop();

	if (const auto* bullet = dynamic_cast<Bullet*>(bulletAsBase.get());
		bulletAsBase.get() != nullptr && bullet != nullptr)
	{
		// std::cout << "[" << GetCurrentTimeString() << "] "
		// 		<< "[" << (_gameMode == PlayAsHost ? "SERVER" : "CLIENT") << "] "
		// 		<< "Bullet REUSED and Bullet pool size =" << _bullets.size()
		// 		<< ", Author=" << author
		// 		<< ", Direction=" << static_cast<int>(dir)
		// 		<< ", Fraction=" << fraction
		// 		<< ", UUID=" << bullet->GetUuid()
		// 		<< std::endl;
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

	std::lock_guard<std::mutex> lock(_bulletsMutex);
	if (const auto* bulletCast = dynamic_cast<Bullet*>(bullet); bulletCast != nullptr)
	{
		// std::cout << "[" << GetCurrentTimeString() << "] "
		// 		<< "[" << (_gameMode == PlayAsHost ? "SERVER" : "CLIENT") << "] "
		// 		<< "Bullet RETURNED to pool and Bullet pool size =" << _bullets.size()
		// 		<< ", Author=" << bulletCast->GetAuthor()
		// 		<< ", UUID=" << bulletCast->GetUuid()
		// 		<< std::endl;

		bulletCast->Disable();
		_bullets.emplace(std::shared_ptr<BaseObj>(bullet, [this](BaseObj* b)
		{
			ReturnBullet(b);
		}));

		_events->EmitEvent<const boost::uuids::uuid>("ServerSend_Dispose", bulletCast->GetUuid());
	}
}

void BulletPool::Clear()
{
	std::lock_guard<std::mutex> lock(_bulletsMutex);
	_isClearing = true;

	// std::cout << "[" << GetCurrentTimeString() << "] "
	// 		<< "[" << (_gameMode == PlayAsHost ? "SERVER" : "CLIENT") << "] "
	// 		<< "Bullet pool CLEARED, bullets in pool: " << _bullets.size()
	// 		<< std::endl;

	while (!_bullets.empty())
	{
		_bullets.pop();
	}

	_isClearing = false;
}
