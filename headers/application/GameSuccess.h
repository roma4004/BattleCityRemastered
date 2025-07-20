#pragma once

#include "Point.h"
#include "interfaces/IGame.h"
#include <SDL.h>
#include <chrono>

enum class GameMode : char8_t;
class INetworkNode;
class Menu;
class BaseObj;
class BulletPool;
class Client;
class EventSystem;
class GameStatistics;
class Server;
class IDrawable;
class UserInput;
class TankSpawner;
class BonusSpawner;
class ObstacleSpawner;
class TextureManager;
class BonusEffectManager;
class SpawnDelayManager;

class GameSuccess final : public IGame
{
	UPoint _windowSize{};
	std::string _name{"Game"};

	std::unique_ptr<INetworkNode> _networkNode{nullptr};
	std::unique_ptr<Menu> _menu{nullptr};
	std::shared_ptr<GameStatistics> _statistics{nullptr};
	std::shared_ptr<EventSystem> _events{nullptr};
	std::shared_ptr<BulletPool> _bulletPool{nullptr};
	std::shared_ptr<TextureManager> _textureManager{nullptr};
	std::shared_ptr<UserInput> _userInput{nullptr};
	std::shared_ptr<TankSpawner> _tankSpawner{nullptr};
	std::shared_ptr<BonusSpawner> _bonusSpawner{nullptr};
	std::shared_ptr<ObstacleSpawner> _obstacleSpawner{nullptr};
	std::shared_ptr<SpawnDelayManager> _spawnDelayManager{nullptr};

	std::vector<std::shared_ptr<BaseObj>> _allObjects{};

	std::chrono::duration<double> _targetFrameDuration{};
	SDL_TimerID _frameTimer{};
	const int _targetFps{60};

	bool _isVsyncOn{};//TODO: add settings inGame for tweak this in real time
	GameMode _selectedGameMode{};
	GameMode _gameMode{};
	bool _frameReady{true};

	void Subscribe();
	void Unsubscribe() const;
	void LoadMap() const;

	void ResetBattlefield(GameMode gameMode);
	void PrevGameMode();
	void NextGameMode();

	[[nodiscard]] Uint32 CountFpsAndDeltaTime(float& deltaTime,
	                                          const std::chrono::high_resolution_clock::time_point& startFrameTime);

	void DisposeDeadObject();

	void OnClientReady() const;

	void MainLoop() override;

	[[nodiscard]] int Result() const override;

	[[nodiscard]] GameMode GetCurrentGameMode() const;
	void SetCurrentGameMode(GameMode selectedGameMode);
	void OnGameModeChangedTo(GameMode newGameMode);

public:
	GameSuccess(UPoint windowSize, std::shared_ptr<EventSystem> events, std::shared_ptr<GameStatistics> statistics,
	            std::unique_ptr<Menu> menu, std::shared_ptr<TextureManager> textureManager, bool isVsyncOn,
	            std::shared_ptr<BonusEffectManager> bonusEffectManager,
	            std::shared_ptr<SpawnDelayManager> spawnDelayManager);

	~GameSuccess() override;
};
