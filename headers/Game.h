#pragma once

#include "BaseObj.h"
#include "EventSystem.h"
#include "MouseButton.h"
#include "Point.h"

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

class Game
{
public:
	Game(const Game& other) = delete;
	Game(Game&& other) noexcept = delete;
	Game() = default;
	virtual ~Game() = default;

	virtual void MainLoop() = 0;
	[[nodiscard]] virtual int Result() const = 0;

	Game& operator=(const Game& other) = delete;
	Game& operator=(Game&& other) noexcept = delete;
};

struct GameStats
{
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
};

struct MenuKeys
{
	bool up{false};
	bool down{false};
	bool reset{false};
	bool menuShow{false};
	bool pause{false};
};

class GameSuccess final : public Game
{
	UPoint _windowSize{0, 0};
	GameMode currentMode{Demo};
	GameStats statistics;
	MenuKeys menuKeys;
	std::string name = "game";

	int* _windowBuffer{nullptr};
	SDL_Renderer* _renderer{nullptr};
	SDL_Texture* _screen{nullptr};

	//fps
	TTF_Font* _fpsFont{nullptr};
	SDL_Surface* _fpsSurface{nullptr};
	SDL_Texture* _fpsTexture{nullptr};

	MouseButtons mouseButtons{};
	std::vector<std::shared_ptr<BaseObj>> allPawns;

	std::shared_ptr<EventSystem> _events;

	std::random_device _rd;

public:
	GameSuccess(UPoint windowSize, int* windowBuffer, SDL_Renderer* renderer, SDL_Texture* screen, TTF_Font* fpsFont);
	~GameSuccess() override;

	void SpawnEnemyTanks(float gridOffset, float speed, int health, float size);
	void ToggleMenu();
	void ResetStatistics();
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

	void SpawnEnemy(bool& needSpawn, int enemyIndex, float gridOffset, float speed, int health, float size);
	void SpawnPlayer1(float gridOffset, float speed, int health, float size);
	void SpawnPlayer2(float gridOffset, float speed, int health, float size);
	void SpawnCoop1(float gridOffset, float speed, int health, float size);
	void SpawnCoop2(float gridOffset, float speed, int health, float size);
	void RespawnTanks();

	void MainLoop() override;

	[[nodiscard]] int Result() const override { return 0; }
};

class GameFailure final : public Game
{
public:
	void MainLoop() override {}

	[[nodiscard]] int Result() const override { return 1; }
};
