#pragma once

#include "Point.h"
#include "interfaces/IGame.h"
#include <SDL.h>
#include <SDL_ttf.h>
#include <chrono>
#include <random>
#include <unordered_map>

enum GameMode : char8_t;
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

class GameSuccess final : public IGame
{
	GameMode _selectedGameMode{};
	GameMode _gameMode{};
	UPoint _windowSize{};
	std::string _name{"Game"};

	std::unique_ptr<INetworkNode> _networkNode{nullptr};
	std::unique_ptr<Menu> _menu{nullptr};
	std::shared_ptr<GameStatistics> _statistics{nullptr};

	std::shared_ptr<SDL_Renderer> _renderer{nullptr};
	std::shared_ptr<SDL_Texture> _screen{nullptr};

	std::shared_ptr<TTF_Font> _fpsFont{nullptr};
	std::shared_ptr<SDL_Texture> _fpsTexture{nullptr};

	std::vector<std::shared_ptr<BaseObj>> _allObjects{};

	std::shared_ptr<EventSystem> _events{nullptr};

	std::shared_ptr<BulletPool> _bulletPool{nullptr};

	std::shared_ptr<TextureManager> _textureManager{nullptr};

	std::unordered_map<int, std::shared_ptr<SDL_Texture>> _fpsTextures{};// pregenerated fps texture

	std::shared_ptr<UserInput> _userInput{nullptr};
	std::shared_ptr<TankSpawner> _tankSpawner{nullptr};
	std::shared_ptr<BonusSpawner> _bonusSpawner{nullptr};
	std::shared_ptr<ObstacleSpawner> _obstacleSpawner{nullptr};

	std::random_device _rd{};

	bool _isVsyncOn{false};//TODO: add settings inGame for tweak this in real time
	const int _targetFPS{60};
	std::chrono::duration<double> _targetFrameDuration{};

	SDL_TimerID _frameTimer{0};
	bool _frameReady{true};

	void Subscribe();
	void Unsubscribe() const;
	void LoadMap() const;

	void ResetBattlefield(GameMode gameMode);
	void PrevGameMode();
	void NextGameMode();
	void GenerateFpsTextures();

	void CountFpsAndDeltaTime(float& deltaTime, const std::chrono::high_resolution_clock::time_point& startFrameTime);

	void DisposeDeadObject();

	void OnClientReady() const;

	void MainLoop() override;

	[[nodiscard]] int Result() const override;

	[[nodiscard]] GameMode GetCurrentGameMode() const;
	void SetCurrentGameMode(GameMode selectedGameMode);

public:
	GameSuccess(UPoint windowSize, std::shared_ptr<SDL_Renderer> renderer, std::shared_ptr<SDL_Texture> screen,
	            std::shared_ptr<TTF_Font> fpsFont, std::shared_ptr<EventSystem> events,
	            std::shared_ptr<GameStatistics> statistics, std::unique_ptr<Menu> menu,
	            std::shared_ptr<TextureManager> textureManager, bool isVsyncOn,
	            std::shared_ptr<BonusEffectManager> bonusEffectManager);

	~GameSuccess() override;
};
