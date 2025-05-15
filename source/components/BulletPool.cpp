#include "../../headers/components/BulletPool.h"
#include "../../headers/components/EventSystem.h"
#include "../../headers/pawns/Bullet.h"
#include "../../headers/pawns/PawnProperty.h"

BulletPool::BulletPool(std::shared_ptr<EventSystem> events, std::vector<std::shared_ptr<BaseObj>>* allObjects,
                       std::shared_ptr<Window> window, const GameMode gameMode)
	: _events{std::move(events)},
	  _name{"BulletPool"},
	  _gameMode{gameMode},
	  _allObjects{allObjects},
	  _window{std::move(window)}
{
	Subscribe();
}

BulletPool::~BulletPool()
{
	Unsubscribe();
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
	const int bulletId = _lastId.load();
	std::string name{"Bullet" + std::to_string(bulletId)};// + author}; //TODO: use bulletIdWithAuthorName
	// TODO: separated bullet pool for each tank or client receive store in order and process one by one from task list
	BaseObjProperty baseObjProperty{
			std::move(rect), color, health, true, bulletId, std::move(name), std::move(fraction)};
	PawnProperty pawnProperty{
			std::move(baseObjProperty), _window, dir, speed, _allObjects, _events, tier, _gameMode};
	return std::shared_ptr<Bullet>(
			new Bullet{std::move(pawnProperty), damage, aoeRadius, std::move(author)},
			[this](Bullet* b)
			{
				ReturnBullet(b);
			});
}

void BulletPool::SpawnBullet(ObjRectangle rect, const int damage, const double aoeRadius, const int color,
                            const int health, const Direction dir, const float speed, std::string author,
                            std::string fraction, const int tier)
{
	std::lock_guard<std::mutex> lock(_bulletsMutex);

	if (_bullets.empty())
	{
		_allObjects->emplace_back(
				CreateNewBullet(rect, damage, aoeRadius, color, health, dir, speed, author, fraction, tier));

		_lastId.fetch_add(1);//TODO: do not reuse index, just spawn with new id

		return;
	}

	std::shared_ptr<BaseObj> bulletAsBase = _bullets.front();
	_bullets.pop();
	if (auto* bullet = dynamic_cast<Bullet*>(bulletAsBase.get()); bullet != nullptr)
	{
		const int bulletId = _lastId.load();
		bullet->SetId(bulletId);
		bullet->Reset(
				std::move(rect), damage, aoeRadius, color, speed, dir, health, std::move(author), std::move(fraction),
				tier);
	}

	_allObjects->emplace_back(bulletAsBase);
}

void BulletPool::ReturnBullet(BaseObj* bullet)
{
	if (_isClearing)
	{
		return;
	}

	if (const auto* bulletCast = dynamic_cast<Bullet*>(bullet); bulletCast != nullptr)
	{
		std::lock_guard<std::mutex> lock(_bulletsMutex);

		bulletCast->Disable();
		_bullets.emplace(std::shared_ptr<BaseObj>(bullet, [this](BaseObj* b)
		{
			ReturnBullet(b);
		}));

		_events->EmitEvent<const int>("ServerSend_Dispose", bulletCast->GetId());
	}
}

void BulletPool::Clear()
{
	_isClearing = true;

	while (!_bullets.empty())
	{
		_bullets.pop();
	}

	_isClearing = false;
	_lastId = 0;
}

//TODO: write statistics for pickuped bonuses
