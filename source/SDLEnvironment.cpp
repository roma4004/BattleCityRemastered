#include "../headers/SDLEnvironment.h"
#include "../headers/ConfigFailure.h"
#include "../headers/ConfigSuccess.h"

#include <memory>

class Config;

SDLEnvironment::SDLEnvironment(const UPoint windowSize) : windowSize{windowSize} {}

SDLEnvironment::~SDLEnvironment()
{
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);

	delete windowBuffer;

	// TTF_CloseFont(font);

	SDL_Quit();
}

[[nodiscard]] std::unique_ptr<Config> SDLEnvironment::Init()
{
	if (SDL_Init(SDL_INIT_EVERYTHING) != 0)
	{
		return std::make_unique<ConfigFailure>("SDL_Init Error: ", SDL_GetError());
	}

	window = SDL_CreateWindow("Battle City remastered", 100, 100, static_cast<int>(windowSize.x),
							  static_cast<int>(windowSize.y), SDL_WINDOW_SHOWN);
	if (window == nullptr)
	{
		return std::make_unique<ConfigFailure>("SDL_CreateWindow Error", SDL_GetError());
	}

	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED /* | SDL_RENDERER_PRESENTVSYNC*/);
	if (renderer == nullptr)
	{
		return std::make_unique<ConfigFailure>("SDL_CreateRenderer Error", SDL_GetError());
	}

	screen = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET,
							   static_cast<int>(windowSize.x), static_cast<int>(windowSize.y));
	if (screen == nullptr)
	{
		return std::make_unique<ConfigFailure>("Screen SDL_CreateTexture Error", SDL_GetError());
	}

	// if (TTF_Init() == -1)
	// {
	// 	return std::make_unique<InitFailure>("TTF_Init Error", TTF_GetError());
	// }
	//
	// if (font = TTF_OpenFont(R"(Resources\Fonts\Roboto\Roboto-Black.ttf)", 14);
	// 	font == nullptr)
	// {
	// 	return std::make_unique<InitFailure>("TTF font loading Error", TTF_GetError());
	// }

	const auto size = windowSize.x * windowSize.y;
	windowBuffer = new int[size];

	return std::make_unique<ConfigSuccess>(windowSize, windowBuffer, renderer, screen /*, font*/);
}
