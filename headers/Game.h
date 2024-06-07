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
	CoopAI,

	EndIterator // should be the last one
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

class GameSuccess final : public Game
{
	UPoint _windowSize{0, 0};
	GameMode currentMode{Demo};
	bool up{false};
	bool down{false};
	bool reset{false};
	bool menuShow{true};

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
	void ResetBattlefield();
	void SetGameMode(GameMode gameMode);
	void PrevGameMode();
	void NextGameMode();
	GameSuccess(UPoint windowSize, int* windowBuffer, SDL_Renderer* renderer, SDL_Texture* screen, TTF_Font* fpsFont);

	void ClearBuffer() const;

	void MouseEvents(const SDL_Event& event);
	void KeyPressed(const SDL_Event& event) const;
	void KeyReleased(const SDL_Event& event) const;
	void KeyboardEvents(const SDL_Event& event) const;
	void textToRender(SDL_Renderer* renderer, Point menuPos, SDL_Color menuColor, const std::string& rowZero) const;
	void HandleMenuText(SDL_Renderer* renderer, UPoint menuBackgroundPos);

	void MainLoop() override;

	[[nodiscard]] int Result() const override { return 0; }
};

class GameFailure final : public Game
{
public:
	void MainLoop() override {}

	[[nodiscard]] int Result() const override { return 1; }
};
