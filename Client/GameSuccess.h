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
#include "../headers/bonuses/BonusSystem.h"
#include "../headers/input/InputProviderForMenu.h"
#include "../headers/interfaces/IGame.h"

#include <SDL.h>
#include <SDL_ttf.h>
#include <random>

class GameSuccess final : public IGame
{
	UPoint _windowSize{0, 0};
	GameMode _selectedGameMode;
	GameMode _currentMode;
	std::shared_ptr<GameStatistics> _statistics;
	Menu _menu;
	std::shared_ptr<TankSpawner> _tankSpawner;
	std::string _name = "Game";

	int* _windowBuffer{nullptr};
	SDL_Renderer* _renderer{nullptr};
	SDL_Texture* _screen{nullptr};

	//fps
	TTF_Font* _fpsFont{nullptr};
	SDL_Surface* _fpsSurface{nullptr};
	SDL_Texture* _fpsTexture{nullptr};

	MouseButtons _mouseButtons{};
	std::vector<std::shared_ptr<BaseObj>> _allObjects;

	std::shared_ptr<EventSystem> _events;

	std::shared_ptr<BulletPool> _bulletPool;

	BonusSystem _bonusSystem;

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

	void EventHandling();
	void DisposeDeadObject();

	void MainLoop() override;

	[[nodiscard]] int Result() const override;

public:
	GameSuccess(UPoint windowSize, int* windowBuffer, SDL_Renderer* renderer, SDL_Texture* screen, TTF_Font* fpsFont,
				SDL_Texture* logoTexture, const std::shared_ptr<EventSystem>& events,
				std::unique_ptr<InputProviderForMenu>& menuInput,
	            const std::shared_ptr<GameStatistics>& statistics);

	~GameSuccess() override;
};
