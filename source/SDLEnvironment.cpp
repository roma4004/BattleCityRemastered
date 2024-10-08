#include "../headers/SDLEnvironment.h"
#include "../headers/ConfigFailure.h"
#include "../headers/ConfigSuccess.h"

#include <memory>

class IConfig;

SDLEnvironment::SDLEnvironment(const UPoint windowSize, const char* fpsFontName, const char* logoName)
	: windowSize{windowSize}, _fpsFontPathName{fpsFontName}, _logoPathName{logoName} {}

SDLEnvironment::~SDLEnvironment()
{
	SDL_DestroyRenderer(renderer);
	renderer = nullptr;
	SDL_DestroyWindow(window);
	window = nullptr;

	delete windowBuffer;
	windowBuffer = nullptr;

	TTF_CloseFont(_fpsFont);
	_fpsFont = nullptr;
	TTF_Quit();

	//TODO: destroy all img surface and texture before it
    SDL_DestroyTexture(_logoTexture);
	IMG_Quit();

	SDL_Quit();
}

[[nodiscard]] std::unique_ptr<IConfig> SDLEnvironment::Init()
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

	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
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

	if (TTF_Init() == -1)
	{
		return std::make_unique<ConfigFailure>("TTF_Init Error", TTF_GetError());
	}

	if (_fpsFont = TTF_OpenFont(_fpsFontPathName, 14); _fpsFont == nullptr)
	{
		return std::make_unique<ConfigFailure>("TTF font loading Error", TTF_GetError());
	}

	if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) {
		return std::make_unique<ConfigFailure>("IMG_Init Error", IMG_GetError());
	}

	SDL_Surface* logoSurface = IMG_Load(_logoPathName);
	if (logoSurface == nullptr) {
		return std::make_unique<ConfigFailure>("IMG Logo Loading Error", IMG_GetError());
	}

	_logoTexture = SDL_CreateTextureFromSurface(renderer, logoSurface);
	SDL_FreeSurface(logoSurface);
	if (_logoTexture == nullptr) {
		return std::make_unique<ConfigFailure>("IMG Logo Texture Creating Error", IMG_GetError());
	}

	const auto size = windowSize.x * windowSize.y;
	windowBuffer = new int[size];

	return std::make_unique<ConfigSuccess>(windowSize, windowBuffer, renderer, screen, _fpsFont, _logoTexture);
}
