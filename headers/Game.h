#pragma once
#include "BaseObj.h"
#include "EventSystem.h"
#include "Map.h"
#include "MouseButton.h"
#include "Pawn.h"
#include "PlayerOne.h"
#include "PlayerTwo.h"

#include <SDL.h>
#include <algorithm>
#include <iostream>
#include <vector>
// #include <SDL_ttf.h>

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
	size_t _windowWidth;
	size_t _windowHeight;

	int* _windowBuffer;
	SDL_Renderer* _renderer;
	SDL_Texture* _screen;
	// TTF_Font* _font;
	// SDL_Surface* fpsSurface{nullptr};
	// SDL_Texture* fpsTexture{nullptr};

	//fps
	MouseButtons mouseButtons;
	std::vector<std::shared_ptr<BaseObj>> allPawns;

	std::shared_ptr<EventSystem> _events;

public:
	GameSuccess(const size_t windowWidth, const size_t windowHeight, int* windowBuffer, SDL_Renderer* renderer,
				SDL_Texture* screen /*, TTF_Font* font*/);

	void ClearBuffer() const;

	void MouseEvents(const SDL_Event& event);

	void MainLoop() override;

	[[nodiscard]] int Result() const override { return 0; }
};

class GameFailure final : public Game
{
public:
	void MainLoop() override {}

	[[nodiscard]] int Result() const override { return 1; }
};
