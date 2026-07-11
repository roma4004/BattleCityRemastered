#pragma once
#include <SDL.h>//NOTE: do not replace with forward declaration, required for minGW
#include <memory>

class IConfig;
class GameConfig;

struct SDLEnvironment final
{
	GameConfig& gameConfig;

	explicit SDLEnvironment(GameConfig& gameConfig);
	~SDLEnvironment();

	[[nodiscard]] std::unique_ptr<IConfig> Init() const;

private:
	[[nodiscard]] std::unique_ptr<SDL_Window, decltype(&SDL_DestroyWindow)> InitWindow() const;
	[[nodiscard]] std::shared_ptr<SDL_Renderer> InitRender() const;
};
