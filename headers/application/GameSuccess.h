#pragma once

#include "Point.h"
#include "components/EventSystem.h"
#include "interfaces/IGame.h"
#include <chrono>

enum class GameMode : char8_t;
class INetworkNode;
class IDrawable;
class BaseObj;
class Menu;
class UserInput;
class TextureManager;
class GameStateManager;
class FramePerSecondManager;
class SpawnManager;
class RenderManager;
class EventSystem;
class BonusEffectManager;
class ScoreBoard;
class GameStatistics;
class RightSideBar;
class GameConfig;

class GameSuccess final : public IGame
{
public:
	GameSuccess(GameConfig& gameConfig, const std::shared_ptr<EventSystem>& events,
				std::unique_ptr<Menu>& menu, std::unique_ptr<RenderManager>& renderManager, GameMode gameMode);

	//NOTE: defaulted out-of-line in the .cpp (not here) - this header only forward-declares the
	//manager types held by unique_ptr below, so an in-header default would need them complete here.
	~GameSuccess() override;

	void MainLoop() override;

	[[nodiscard]] int Result() const override;

private:
	void Subscribe();

	void ApplyGameMode(GameMode gameMode);
	void PrevGameMode();
	void NextGameMode();

	void DisposeDeadObject();
	void FlushSpawnQueue();

	void OnClientReady() const;

	[[nodiscard]] GameMode GetCurrentGameMode() const;
	void SetCurrentGameMode(GameMode selectedGameMode);
	void OnGameModeChangedTo(GameMode newGameMode);

	UPoint _windowSize{};
	std::string _name{"Game"};

	std::unique_ptr<INetworkNode> _networkNode{nullptr};
	std::unique_ptr<Menu> _menu{nullptr};
	std::unique_ptr<TextureManager> _textureManager{nullptr};
	std::unique_ptr<GameStateManager> _stateManager{nullptr};
	std::unique_ptr<UserInput> _userInput{nullptr};
	std::unique_ptr<FramePerSecondManager> _fpsManager{nullptr};
	std::unique_ptr<SpawnManager> _spawnManager{nullptr};
	std::unique_ptr<RenderManager> _renderManager{nullptr};
	std::unique_ptr<BonusEffectManager> _bonusEffectManager{nullptr};
	std::unique_ptr<ScoreBoard> _scoreBoard{nullptr};
	std::unique_ptr<RightSideBar> _rightSideBar{nullptr};

	std::shared_ptr<EventSystem> _events{nullptr};
	std::vector<EventSubscription> _subs{};
	// Toggled at runtime on every GameModeChangedToEvent (host-only listener), independent of
	// _subs's fixed subscribe-once-at-construction lifetime - assigning a new EventSubscription
	// here auto-unsubscribes whatever was previously held.
	EventSubscription _clientReadySub{};
	//TODO: modify only under mutex lock (main and network thread can add)
	std::vector<std::shared_ptr<BaseObj>> _allObjects{};
	std::vector<std::shared_ptr<BaseObj>> _pendingSpawns{};

	GameMode _selectedGameMode{};
	GameMode _gameMode{};
	double _deltaTime{};
};
