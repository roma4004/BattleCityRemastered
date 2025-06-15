#pragma once

#include "../BonusEffectProperty.h"
#include "../enums/Direction.h"
#include "../input/InputProviderForPlayerOne.h"
#include "../input/InputProviderForPlayerOneNet.h"
#include "../input/InputProviderForPlayerTwo.h"
#include "../input/InputProviderForPlayerTwoNet.h"

template<typename TTankType>
void TankSpawner::RespawnTank(const ObjRectangle rect, int color, int health, std::string name, std::string fraction,
                              const float speed, buuid uuid, BonusEffectProperty effects)
{
	BaseObjProperty baseObjProperty{
			std::move(rect), color, health, true, uuid, std::move(name), std::move(fraction)};
	PawnProperty pawnProperty{
			std::move(baseObjProperty), _allObjects, _events, _windowSize, _gameMode, 1, UP, speed};

	if (auto tank = std::make_shared<TTankType>(std::move(pawnProperty), _bulletPool, effects);
		tank.get() != nullptr)
	{
		_allObjects->emplace_back(tank);
	}
}

template<>
inline void TankSpawner::RespawnTank<Player>(const ObjRectangle rect, int color, int health, std::string name,
                                             std::string fraction, const float speed, buuid uuid,
                                             BonusEffectProperty effects)
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
			std::move(baseObjProperty), _allObjects, _events, _windowSize, _gameMode, 1, UP, speed};

	if (auto tank =
				std::make_shared<Player>(std::move(pawnProperty), _bulletPool, std::move(inputProvider), effects);
		tank.get() != nullptr)
	{
		_allObjects->emplace_back(tank);
	}
}
