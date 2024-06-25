#include "../headers/GameStatistics.h"

GameStatistics::GameStatistics(std::string name, std::shared_ptr<EventSystem> events)
	: _name{std::move(name)}, _events{std::move(events)}
{
	_events->AddListener("Enemy1_Died", _name, [this]() { enemyOneNeedRespawn = true; });
	_events->AddListener("Enemy2_Died", _name, [this]() { enemyTwoNeedRespawn = true; });
	_events->AddListener("Enemy3_Died", _name, [this]() { enemyThreeNeedRespawn = true; });
	_events->AddListener("Enemy4_Died", _name, [this]() { enemyFourNeedRespawn = true; });

	_events->AddListener("P1_Died", _name, [this]() { playerOneNeedRespawn = true; });
	_events->AddListener("P2_Died", _name, [this]() { playerTwoNeedRespawn = true; });
	_events->AddListener("CoopOneAI_Died", _name, [this]() { coopOneAINeedRespawn = true; });
	_events->AddListener("CoopTwoAI_Died", _name, [this]() { coopTwoAINeedRespawn = true; });

	_events->AddListener("P1_Spawn", _name, [this]()
	{
		playerOneNeedRespawn = false;
		--playerOneRespawnResource;
	});
	_events->AddListener("P2_Spawn", _name, [this]()
	{
		playerTwoNeedRespawn = false;
		--playerTwoRespawnResource;
	});
	_events->AddListener("CoopOneAI_Spawn", _name, [this]()
	{
		coopOneAINeedRespawn = false;
		--playerOneRespawnResource;
	});
	_events->AddListener("CoopTwoAI_Spawn", _name, [this]()
	{
		coopTwoAINeedRespawn = false;
		--playerTwoRespawnResource;
	});

	_events->AddListener("Enemy1_Spawn", _name, [this]()
	{
		enemyOneNeedRespawn = false;
		--enemyRespawnResource;
	});
	_events->AddListener("Enemy2_Spawn", _name, [this]()
	{
		enemyTwoNeedRespawn = false;
		--enemyRespawnResource;
	});
	_events->AddListener("Enemy3_Spawn", _name, [this]()
	{
		enemyThreeNeedRespawn = false;
		--enemyRespawnResource;
	});
	_events->AddListener("Enemy4_Spawn", _name, [this]()
	{
		enemyFourNeedRespawn = false;
		--enemyRespawnResource;
	});
}

GameStatistics::~GameStatistics()
{
	_events->RemoveListener("Enemy1_Died", _name);
	_events->RemoveListener("Enemy2_Died", _name);
	_events->RemoveListener("Enemy3_Died", _name);
	_events->RemoveListener("Enemy4_Died", _name);

	_events->RemoveListener("P1_Died", _name);
	_events->RemoveListener("P2_Died", _name);
	_events->RemoveListener("CoopOneAI_Died", _name);
	_events->RemoveListener("CoopTwoAI_Died", _name);

	_events->RemoveListener("P1_Spawn", _name);
	_events->RemoveListener("P2_Spawn", _name);
	_events->RemoveListener("CoopOneAI_Spawn", _name);
	_events->RemoveListener("CoopTwoAI_Spawn", _name);

	_events->RemoveListener("Enemy1_Spawn", _name);
	_events->RemoveListener("Enemy2_Spawn", _name);
	_events->RemoveListener("Enemy3_Spawn", _name);
	_events->RemoveListener("Enemy4_Spawn", _name);
}

void GameStatistics::ResetStatistics()
{
	enemyRespawnResource = 20;
	playerOneRespawnResource = 3;
	playerTwoRespawnResource = 3;

	enemyOneNeedRespawn = false;
	enemyTwoNeedRespawn = false;
	enemyThreeNeedRespawn = false;
	enemyFourNeedRespawn = false;

	playerOneNeedRespawn = false;
	playerTwoNeedRespawn = false;
	coopOneAINeedRespawn = false;
	coopTwoAINeedRespawn = false;
}
