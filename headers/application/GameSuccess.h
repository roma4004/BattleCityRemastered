#pragma once

#include "UserInput.h"
#include "../GameMode.h"
#include "../Menu.h"
#include "../bonuses/BonusSpawner.h"
#include "../interfaces/IGame.h"
#include "../obstacles/ObstacleSpawner.h"

#include <SDL.h>
#include <SDL_ttf.h>
#include <random>

struct Server;
class BaseObj;
class BulletPool;
class InputProviderForMenu;
class EventSystem;
class TankSpawner;
class GameStatistics;
//class Client;

class GameSuccess final : public IGame
{
	GameMode _selectedGameMode{OnePlayer};
	GameMode _currentMode{Demo};
	std::string _name{"Game"};

	std::unique_ptr<Menu> _menu{nullptr};
	std::shared_ptr<GameStatistics> _statistics{nullptr};
	std::shared_ptr<TankSpawner> _tankSpawner{nullptr};

	std::shared_ptr<Window> _window{nullptr};
	std::shared_ptr<SDL_Renderer> _renderer{nullptr};
	std::shared_ptr<SDL_Texture> _screen{nullptr};

	std::shared_ptr<TTF_Font> _fpsFont{nullptr};
	std::shared_ptr<SDL_Texture> _fpsTexture{nullptr};

	std::vector<std::shared_ptr<BaseObj>> _allObjects;

	std::shared_ptr<EventSystem> _events{nullptr};

	std::shared_ptr<BulletPool> _bulletPool{nullptr};

	UserInput _userInput;
	BonusSpawner _bonusSpawner;
	ObstacleSpawner _obstacleSpawner;

	std::random_device _rd;

	void Subscribe();
	void Unsubscribe() const;

	void ResetBattlefield(GameMode gameMode);
	void PrevGameMode();
	void NextGameMode();

	void HandleFPS(Uint32& frameCount, Uint64& fpsPrevUpdateTime, Uint32& fps, Uint64 newTime);

	void DisposeDeadObject();

	void MainLoop() override;

	[[nodiscard]] int Result() const override;

	[[nodiscard]] GameMode GetCurrentGameMode() const;
	void SetCurrentGameMode(GameMode selectedGameMode);

public:
	GameSuccess(std::shared_ptr<Window> window, std::shared_ptr<SDL_Renderer> renderer,
	            std::shared_ptr<SDL_Texture> screen, std::shared_ptr<TTF_Font> fpsFont,
	            std::shared_ptr<EventSystem> events, std::shared_ptr<GameStatistics> statistics,
	            std::unique_ptr<Menu> menu);

	~GameSuccess() override;
};
