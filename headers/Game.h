#pragma once

#include "BaseObj.h"
#include "EventSystem.h"
#include "MouseButton.h"

#include <SDL.h>
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
	size_t _windowWidth{0};
	size_t _windowHeight{0};

	int* _windowBuffer{nullptr};
	SDL_Renderer* _renderer{nullptr};
	SDL_Texture* _screen{nullptr};

	//fps
	// TTF_Font* _font;
	// SDL_Surface* fpsSurface{nullptr};
	// SDL_Texture* fpsTexture{nullptr};

	MouseButtons mouseButtons{};
	std::vector<std::shared_ptr<BaseObj>> allPawns;

	std::shared_ptr<EventSystem> _events;

public:
	GameSuccess(size_t windowWidth, size_t windowHeight, int* windowBuffer, SDL_Renderer* renderer,
				SDL_Texture* screen /*, TTF_Font* font*/);

	void ClearBuffer() const;

	void MouseEvents(const SDL_Event& event);
	void KeyPressed(const SDL_Event& event) const;
	void KeyReleased(const SDL_Event& event) const;
	void KeyboardEvents(const SDL_Event& event) const;

	void MainLoop() override;

	[[nodiscard]] int Result() const override { return 0; }
};

class GameFailure final : public Game
{
public:
	void MainLoop() override {}

	[[nodiscard]] int Result() const override { return 1; }
};
