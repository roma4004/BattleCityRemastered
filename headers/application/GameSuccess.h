#pragma once

#include "Point.h"
#include "components/managers/BonusEffectManager.h"
#include "interfaces/IGame.h"
#include <chrono>

enum class GameMode : char8_t;
class INetworkNode;
class IDrawable;
class BaseObj;
class Menu;
class UserInput;
class TextureManager;
class StateManager;
class FramePerSecondManager;
class SpawnManager;
class RenderManager;
class EventSystem;

class GameSuccess final : public IGame
{
	UPoint _windowSize{};
	std::string _name{"Game"};

	std::unique_ptr<INetworkNode> _networkNode{nullptr};
	std::unique_ptr<Menu> _menu{nullptr};
	std::unique_ptr<TextureManager> _textureManager{nullptr};
	std::unique_ptr<StateManager> _stateManager{nullptr};
	std::unique_ptr<UserInput> _userInput{nullptr};
	std::unique_ptr<FramePerSecondManager> _fpsManager{nullptr};
	std::unique_ptr<SpawnManager> _spawnManager{nullptr};
	std::unique_ptr<RenderManager> _renderManager{nullptr};
	std::unique_ptr<BonusEffectManager> _bonusEffectManager{nullptr};

	std::shared_ptr<EventSystem> _events{nullptr};
	//TODO: modify only under mutex lock (main and network thread can add)
	std::vector<std::shared_ptr<BaseObj>> _allObjects{};

	GameMode _selectedGameMode{};
	GameMode _gameMode{};
	double _deltaTime{};

	void Subscribe();
	void Unsubscribe() const;

	void ResetBattlefieldTo(GameMode gameMode);
	void PrevGameMode();
	void NextGameMode();

	void DisposeDeadObject();

	void OnClientReady() const;

	void MainLoop() override;

	[[nodiscard]] int Result() const override;

	[[nodiscard]] GameMode GetCurrentGameMode() const;
	void SetCurrentGameMode(GameMode selectedGameMode);
	void OnGameModeChangedTo(GameMode newGameMode);

public:
	GameSuccess(UPoint windowSize, const std::shared_ptr<EventSystem>& events, std::unique_ptr<Menu>& menu,
				bool isVsyncOn, std::unique_ptr<RenderManager>& renderManager);

	~GameSuccess() override;
};
