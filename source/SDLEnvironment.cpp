#include "../headers/SDLEnvironment.h"
#include "../headers/ConfigFailure.h"
#include "../headers/ConfigSuccess.h"

#include <memory>

class Config;

SDLEnvironment::SDLEnvironment(const int windowWidth, const int windowHeight)
	: windowWidth(windowWidth), windowHeight(windowHeight)
{
}

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

	window = SDL_CreateWindow("Battle City remastered", 100, 100, windowWidth, windowHeight, SDL_WINDOW_SHOWN);
	if (window == nullptr)
	{
		return std::make_unique<ConfigFailure>("SDL_CreateWindow Error", SDL_GetError());
	}

	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
	if (renderer == nullptr)
	{
		return std::make_unique<ConfigFailure>("SDL_CreateRenderer Error", SDL_GetError());
	}

	screen = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET, windowWidth, windowHeight);
	if (screen == nullptr)
	{
		return std::make_unique<ConfigFailure>("Screen SDL_CreateTexture Error", SDL_GetError());
	}

	const auto buffer_size = windowWidth * windowHeight;
	windowBuffer = new int[buffer_size];

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

	const auto size = windowWidth * windowHeight;
	windowBuffer = new int[size];

	return std::make_unique<ConfigSuccess>(windowWidth, windowHeight, windowBuffer, renderer, screen /*, font*/);
}
