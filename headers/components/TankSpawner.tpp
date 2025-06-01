#pragma once

template<typename TTankType>
void TankSpawner::RespawnTank(const ObjRectangle rect, int color, int health, std::string name, std::string fraction,
                              const float speed, boost::uuids::uuid uuid)
{
	BaseObjProperty baseObjProperty{
			std::move(rect), color, health, true, uuid, std::move(name), std::move(fraction)};
	PawnProperty pawnProperty{
			std::move(baseObjProperty), _window, UP, speed, _allObjects, _events, 1, _gameMode, _atlasTexture,
			_renderer};

	if (auto tank = std::make_shared<TTankType>(std::move(pawnProperty), _bulletPool);
		tank.get() != nullptr)
	{
		_allObjects->emplace_back(tank);
	}
}

template<>
inline void TankSpawner::RespawnTank<Player>(const ObjRectangle rect, int color, int health, std::string name,
                                             std::string fraction, const float speed, boost::uuids::uuid uuid)
{
	std::unique_ptr<IInputProvider> inputProvider;
	if (name == "Player1")
	{
		if (_gameMode == PlayAsClient)
		{
			inputProvider = std::make_unique<InputProviderForPlayerOneNet>(_events);
		}
		else
		{
			inputProvider = std::make_unique<InputProviderForPlayerOne>(_events);
		}
	}
	else
	{
		if (_gameMode == PlayAsClient || _gameMode == PlayAsHost)
		{
			inputProvider = std::make_unique<InputProviderForPlayerTwoNet>(_events);
		}
		else
		{
			inputProvider = std::make_unique<InputProviderForPlayerTwo>(_events);
		}
	}

	BaseObjProperty baseObjProperty{
			std::move(rect), color, health, true, uuid, std::move(name), std::move(fraction)};
	PawnProperty pawnProperty{
			std::move(baseObjProperty), _window, UP, speed, _allObjects, _events, 1, _gameMode, _atlasTexture,
			_renderer};

	if (auto tank = std::make_shared<Player>(std::move(pawnProperty), _bulletPool, std::move(inputProvider));
		tank.get() != nullptr)
	{
		_allObjects->emplace_back(tank);
	}
}

// Include the template implementation
#include "TankSpawner.tpp"
