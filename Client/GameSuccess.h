#pragma once

#include "Client.h"
#include "../headers/BaseObj.h"
#include "../headers/BulletPool.h"
#include "../headers/EventSystem.h"
#include "../headers/GameMode.h"
#include "../headers/GameStatistics.h"
#include "../headers/Menu.h"
#include "../headers/MouseButton.h"
#include "../headers/Point.h"
#include "../headers/TankSpawner.h"
#include "../headers/bonuses/BonusSpawner.h"
#include "../headers/interfaces/IGame.h"
#include "../headers/obstacles/ObstacleSpawner.h"

#include <SDL.h>
#include <SDL_ttf.h>
#include <random>

class GameSuccess final : public IGame
{
	UPoint _windowSize{.x = 0, .y = 0};
	GameMode _selectedGameMode{Demo};
	GameMode _currentMode{Demo};
	std::shared_ptr<GameStatistics> _statistics{nullptr};
	std::unique_ptr<Menu> _menu{nullptr};
	std::shared_ptr<TankSpawner> _tankSpawner{nullptr};
	std::string _name{"Game"};

	std::shared_ptr<int[]> _windowBuffer{nullptr};
	std::shared_ptr<SDL_Renderer> _renderer{nullptr};
	std::shared_ptr<SDL_Texture> _screen{nullptr};

	//fps
	std::shared_ptr<TTF_Font> _fpsFont{nullptr};
	std::shared_ptr<SDL_Texture> _fpsTexture{nullptr};

	MouseButtons _mouseButtons{};

	std::vector<std::shared_ptr<BaseObj>> _allObjects;

	std::shared_ptr<EventSystem> _events{nullptr};

	std::shared_ptr<BulletPool> _bulletPool{nullptr};

	BonusSpawner _bonusSpawner;
	ObstacleSpawner _obstacleSpawner;

	std::random_device _rd;

	bool _isGameOver{false};
	bool _isPause{false};

	void Subscribe();
	void Unsubscribe() const;

	void ResetBattlefield();
	void PrevGameMode();
	void NextGameMode();

	void ClearBuffer() const;

	void MouseEvents(const SDL_Event& event);
	void KeyPressed(const SDL_Event& event) const;
	void KeyReleased(const SDL_Event& event) const;
	void KeyboardEvents(const SDL_Event& event) const;

	void HandleFPS(Uint32& frameCount, Uint64& fpsPrevUpdateTime, Uint32& fps, Uint64 newTime);

	void UserInputHandling();
	void DisposeDeadObject();

	void MainLoop() override;

	[[nodiscard]] int Result() const override;

	[[nodiscard]] GameMode GetCurrentGameMode() const;
	void SetCurrentGameMode(GameMode selectedGameMode);

public:
	GameSuccess(UPoint windowSize, std::shared_ptr<int[]> windowBuffer, std::shared_ptr<SDL_Renderer> renderer,
	            std::shared_ptr<SDL_Texture> screen, std::shared_ptr<TTF_Font> fpsFont,
	            std::shared_ptr<EventSystem> events, std::shared_ptr<GameStatistics> statistics,
	            std::unique_ptr<Menu> menu);

	~GameSuccess() override;
};
