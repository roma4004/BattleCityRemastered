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
#include <algorithm>
#include <iterator>
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
		_free.push(CreateNewTank());
	}

	Subscribe();
}

TankPool::~TankPool() = default;

void TankPool::Subscribe()
{
	_subs.push_back(_events->AddListener(this, &TankPool::OnGameReset));
	_subs.push_back(_events->AddListener(this, &TankPool::OnPostTickUpdate));
}

void TankPool::OnGameReset(const GameResetEvent&) { Clear(); }

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
	std::shared_ptr<Tank> tank;
	if (_free.empty())
	{
		tank = CreateNewTank();
	}
	else
	{
		tank = _free.front();
		_free.pop();
	}

	tank->Reset(property, std::move(driver));

	_inPlay.push_back(tank);

	return tank;
}

//NOTE: nothing emitted here - the tank announced its own death already
void TankPool::OnPostTickUpdate(const PostTickUpdateEvent&)
{
	auto isWrecked = [](const std::shared_ptr<Tank>& tank) { return !tank->GetIsAlive(); };

	std::vector<std::shared_ptr<Tank>> returned{};
	std::ranges::copy_if(_inPlay, std::back_inserter(returned), isWrecked);
	std::erase_if(_inPlay, isWrecked);

	for (const std::shared_ptr<Tank>& tank: returned)
	{
		//NOTE: a tank in _free must not listen, or the next SpawnTank subscribes it twice
		tank->Deactivate();
		_free.push(tank);
	}

	if (!returned.empty())
	{
		Log::Detail("tanks returned to a pool of " + std::to_string(_free.size()));
	}
}

void TankPool::Clear()
{
	Log::Detail("tank pool cleared, held " + std::to_string(_free.size() + _inPlay.size()));

	_free = {};
	_inPlay.clear();
}
