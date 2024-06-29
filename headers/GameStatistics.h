#pragma once

#include "EventSystem.h"

#include <memory>
#include <string>

class GameStatistics
{
public:
	GameStatistics(std::shared_ptr<EventSystem> events);

	~GameStatistics();

	void Subscribe();
	void Unsubscribe() const;

	void Reset();

	// TODO: use std::atomic when multithreading is used
	int enemyRespawnResource{20};
	int playerOneRespawnResource{3};
	int playerTwoRespawnResource{3};
	bool enemyOneNeedRespawn{false};
	bool enemyTwoNeedRespawn{false};
	bool enemyThreeNeedRespawn{false};
	bool enemyFourNeedRespawn{false};
	bool playerOneNeedRespawn{false};
	bool playerTwoNeedRespawn{false};
	bool coopOneAINeedRespawn{false};
	bool coopTwoAINeedRespawn{false};

private:
	std::string _name;
	std::shared_ptr<EventSystem> _events;
};
