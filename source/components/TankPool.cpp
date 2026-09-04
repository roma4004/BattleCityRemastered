#include "components/TankPool.h"
#include "application/GameConfig.h"
#include "components/EventSystem.h"
#include "components/events/CoreLifecycleEvents.h"
#include "components/events/TimingEvents.h"
#include "components/input/InputProviderForBot.h"
#include "entities/pawns/PawnProperty.h"
#include "entities/pawns/Tank.h"
#include "entities/pawns/TankResetProperty.h"
#include "utils/Log.h"
#include <string>

namespace
{
//NOTE: four enemy seats and two player ones - the field never holds more at once
constexpr std::size_t kSeatCount{6u};
}//namespace

TankPool::TankPool(const std::shared_ptr<EventSystem>& events,
				   const std::vector<std::shared_ptr<BaseObj>>& allObjects, const GameConfig& gameConfig,
				   const std::shared_ptr<BulletPool>& bulletPool)
	: _events{events}
	, _allObjects{allObjects}
	, _gameConfig{gameConfig}
	, _bulletPool{bulletPool}
{
	for (std::size_t i = 0u; i < kSeatCount; ++i)
	{
		_slots.AddFree(CreateNewTank());
	}

	Subscribe();
}

TankPool::~TankPool() = default;

void TankPool::Subscribe()
{
	_subs.push_back(_events->AddListener(this, &TankPool::OnGameReset));
	_subs.push_back(_events->AddListener(this, &TankPool::OnPostTickUpdate));
}

//NOTE: shelved, not dropped - a mode switch changes who fills the seats, not what a tank is made of
void TankPool::OnGameReset(const GameResetEvent&)
{
	_slots.ReclaimAll();

	Log::Detail("tank pool shelved for a new match, free " + std::to_string(_slots.FreeCount()));
}

//NOTE: the driver here is a placeholder - SpawnTank hands the tank its real one before it enters the world
std::shared_ptr<Tank> TankPool::CreateNewTank() const
{
	PawnProperty pawnProperty{.baseObjProperty = {},
							  .allObjects = _allObjects,
							  .events = _events,
							  .gameMode = _gameConfig.gameMode};

	return std::make_shared<Tank>(std::move(pawnProperty), _bulletPool,
								  std::make_unique<InputProviderForBot>(_allObjects, _gameConfig), _gameConfig);
}

std::shared_ptr<Tank> TankPool::SpawnTank(const TankResetProperty& property,
										  std::unique_ptr<IInputProvider> driver)
{
	std::shared_ptr<Tank> tank = _slots.TakeFree();
	if (tank == nullptr)
	{
		tank = CreateNewTank();
	}

	tank->Reset(property, std::move(driver));

	_slots.Track(tank);

	return tank;
}

//NOTE: nothing emitted here - the tank announced its own death already
void TankPool::OnPostTickUpdate(const PostTickUpdateEvent&)
{
	if (const std::vector<std::shared_ptr<Tank>> returned = _slots.ReclaimDead();
		!returned.empty())
	{
		Log::Detail("tanks returned to a pool of " + std::to_string(_slots.FreeCount()));
	}
}
