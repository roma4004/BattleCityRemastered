#pragma once

#include "BaseObj.h"
#include "BulletPool.h"
#include "EventSystem.h"
#include "GameStatistics.h"
#include "InputProviderForMenu.h"
#include "Menu.h"
#include "MouseButton.h"
#include "Point.h"
#include "interfaces/IGame.h"

#include <SDL.h>
#include <SDL_ttf.h>
#include <random>

enum GameMode
{
	Demo,

	OnePlayer,
	TwoPlayers,
	CoopWithAI,

	EndIterator// should be the last one
};

class GameSuccess final : public IGame
{
	UPoint _windowSize{0, 0};
	GameMode _currentMode{Demo};
	std::unique_ptr<GameStatistics> _statistics;
	Menu _menu;
	std::string _name = "game";

	int* _windowBuffer{nullptr};
	SDL_Renderer* _renderer{nullptr};
	SDL_Texture* _screen{nullptr};

	//fps
	TTF_Font* _fpsFont{nullptr};
	SDL_Surface* _fpsSurface{nullptr};
	SDL_Texture* _fpsTexture{nullptr};

	MouseButtons _mouseButtons{};
	std::vector<std::shared_ptr<BaseObj>> _allPawns;

	std::shared_ptr<EventSystem> _events;

	std::shared_ptr<BulletPool> _bulletPool;

	std::random_device _rd;

	bool _isGameOver{false};

public:
	GameSuccess(UPoint windowSize, int* windowBuffer, SDL_Renderer* renderer, SDL_Texture* screen, TTF_Font* fpsFont,
	            std::shared_ptr<EventSystem> events, std::unique_ptr<InputProviderForMenu>& menuInput,
	            std::unique_ptr<GameStatistics>& statistics);

	~GameSuccess() override;

	void SpawnEnemyTanks(float gridOffset, float speed, int health, float size);
	void SpawnPlayerTanks(float gridOffset, float speed, int health, float size);
	void ResetBattlefield();
	void SetGameMode(GameMode gameMode);
	void PrevGameMode();
	void NextGameMode();

	void ClearBuffer() const;

	void MouseEvents(const SDL_Event& event);
	void KeyPressed(const SDL_Event& event) const;
	void KeyReleased(const SDL_Event& event) const;
	void KeyboardEvents(const SDL_Event& event) const;

	void TextToRender(SDL_Renderer* renderer, Point pos, SDL_Color color, const std::string& text) const;
	void HandleMenuText(SDL_Renderer* renderer, UPoint menuBackgroundPos);
	void HandleFPS(Uint32& frameCount, Uint64& fpsPrevUpdateTime, Uint32& fps, Uint64 newTime);

	static bool IsCollideWith(const Rectangle& r1, const Rectangle& r2);

	void SpawnEnemy(int index, float gridOffset, float speed, int health, float size);
	void SpawnPlayer1(float gridOffset, float speed, int health, float size);
	void SpawnPlayer2(float gridOffset, float speed, int health, float size);
	void SpawnCoop1(float gridOffset, float speed, int health, float size);
	void SpawnCoop2(float gridOffset, float speed, int health, float size);
	void RespawnTanks();
	void EventHandling();

	void MainLoop() override;

	[[nodiscard]] int Result() const override;
};
