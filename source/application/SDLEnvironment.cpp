#include "application/SDLEnvironment.h"
#include "application/ConfigFailure.h"
#include "application/ConfigSuccess.h"
#include "application/GameConfig.h"
#include "application/UserInput.h"
#include <SDL.h>
#include <SDL_image.h>
#include <SDL_mixer.h>
#include <SDL_ttf.h>
#include <iostream>
#include <memory>

class IConfig;

SDLEnvironment::SDLEnvironment(const UPoint windowSize, const UPoint windowPos, GameConfig& gameConfig,
							   const char* selectorIcon, const char* xBoxCon, const char* pS5Con)
	: windowSize{windowSize}
	, windowPos{windowPos}
	, gameConfig{gameConfig}
	, selectorIconPathName{selectorIcon}
	, xBoxHintPathName{xBoxCon}
	, pS5HintPathName{pS5Con} {}

SDLEnvironment::~SDLEnvironment()
{
	Mix_CloseAudio();
	TTF_Quit();
	IMG_Quit();
	SDL_Quit();
}

[[nodiscard]] std::unique_ptr<IConfig> SDLEnvironment::Init()
{
	if (SDL_Init(SDL_INIT_EVERYTHING) != 0)
	{
		return std::make_unique<ConfigFailure>("SDL_Init Error: ", SDL_GetError());
	}

	// creating window
	UPoint windowSizeHalf{};
	if (sdlWindow = InitWindow(windowSizeHalf);
		sdlWindow == nullptr)
	{
		return std::make_unique<ConfigFailure>("SDL_CreateWindow Error", SDL_GetError());
	}

	// creating renderer
	if (renderer = InitRender(windowSizeHalf);
		renderer == nullptr)
	{
		return std::make_unique<ConfigFailure>("SDL_CreateRenderer Error", SDL_GetError());
	}

	// font init and loading
	std::shared_ptr<TTF_Font> fontSmall{nullptr};
	std::shared_ptr<TTF_Font> fontMedium{nullptr};
	{
		const auto fontPathName(
				gameConfig.pTreeIni.get<std::string>("Fonts.BattleCity", "Fonts.BattleCity path from config.ini"));
		if (TTF_Init() == -1)
		{
			return std::make_unique<ConfigFailure>("TTF_Init Error", TTF_GetError());
		}

		if (fontSmall = {TTF_OpenFont(fontPathName.c_str(), 14), TTF_CloseFont};
			fontSmall == nullptr)
		{
			return std::make_unique<ConfigFailure>("TTF font loading Error", TTF_GetError());
		}

		if (fontMedium = {TTF_OpenFont(fontPathName.c_str(), 24), TTF_CloseFont};
			fontMedium == nullptr)
		{
			return std::make_unique<ConfigFailure>("TTF font loading Error", TTF_GetError());
		}
	}

	if (!IMG_Init(IMG_INIT_PNG))
	{
		return std::make_unique<ConfigFailure>("IMG_Init Error", IMG_GetError());
	}

	// texture logo loading
	std::shared_ptr<SDL_Texture> logoTexture{nullptr};
	{
		std::shared_ptr<SDL_Surface> logoSurface{nullptr};
		const auto logoPathName(
				gameConfig.pTreeIni.get<std::string>("Images.Logo", "Images.Logo path from config.ini"));
		if (logoSurface = {IMG_Load(logoPathName.c_str()), SDL_FreeSurface};
			logoSurface == nullptr)
		{
			return std::make_unique<ConfigFailure>("IMG Logo Loading Error", IMG_GetError());
		}

		if (logoTexture = {SDL_CreateTextureFromSurface(renderer.get(), logoSurface.get()), SDL_DestroyTexture};
			logoTexture == nullptr)
		{
			return std::make_unique<ConfigFailure>("IMG Logo Texture Creating Error", IMG_GetError());
		}
	}

	// texture joy icon loading
	std::shared_ptr<SDL_Texture> joyIconTexture{nullptr};
	{
		std::shared_ptr<SDL_Surface> joyIconSurface{nullptr};
		if (joyIconSurface = {IMG_Load(selectorIconPathName), SDL_FreeSurface};
			joyIconSurface == nullptr)
		{
			return std::make_unique<ConfigFailure>("IMG Joy icon Loading Error", IMG_GetError());
		}

		if (joyIconTexture = {SDL_CreateTextureFromSurface(renderer.get(), joyIconSurface.get()), SDL_DestroyTexture};
			joyIconTexture == nullptr)
		{
			return std::make_unique<ConfigFailure>("IMG Joy icon Texture Creating Error", IMG_GetError());
		}
	}

	// texture PS5 controls hint loading
	{
		std::string id = "Images.PS5_Create";
		const auto pathName(gameConfig.pTreeIni.get<std::string>(id, id + " path from config.ini"));
		std::shared_ptr<SDL_Surface> surface{nullptr};
		if (surface = {IMG_Load(pathName.c_str()), SDL_FreeSurface};
			surface == nullptr)
		{
			return std::make_unique<ConfigFailure>("IMG " + pathName + " Loading Error", IMG_GetError());
		}

		std::shared_ptr<SDL_Texture> texture{nullptr};
		if (texture = {SDL_CreateTextureFromSurface(renderer.get(), surface.get()), SDL_DestroyTexture};
			texture != nullptr)
		{
			buttonTexturesPS5.push_back(texture);
		}
		else
		{
			return std::make_unique<ConfigFailure>("IMG " + pathName + " Texture Creating Error", IMG_GetError());
		}
	}
	{
		std::string id = "Images.PS5_Cross";
		const auto pathName(gameConfig.pTreeIni.get<std::string>(id, id + " path from config.ini"));
		std::shared_ptr<SDL_Surface> surface{nullptr};
		if (surface = {IMG_Load(pathName.c_str()), SDL_FreeSurface};
			surface == nullptr)
		{
			return std::make_unique<ConfigFailure>("IMG " + pathName + " Loading Error", IMG_GetError());
		}

		std::shared_ptr<SDL_Texture> texture{nullptr};
		if (texture = {SDL_CreateTextureFromSurface(renderer.get(), surface.get()), SDL_DestroyTexture};
			texture != nullptr)
		{
			buttonTexturesPS5.push_back(texture);
		}
		else
		{
			return std::make_unique<ConfigFailure>("IMG " + pathName + " Texture Creating Error", IMG_GetError());
		}
	}
	{
		std::string id = "Images.PS5_D-Pad";
		const auto pathName(gameConfig.pTreeIni.get<std::string>(id, id + " path from config.ini"));
		std::shared_ptr<SDL_Surface> surface{nullptr};
		if (surface = {IMG_Load(pathName.c_str()), SDL_FreeSurface};
			surface == nullptr)
		{
			return std::make_unique<ConfigFailure>("IMG " + pathName + " Loading Error", IMG_GetError());
		}

		std::shared_ptr<SDL_Texture> texture{nullptr};
		if (texture = {SDL_CreateTextureFromSurface(renderer.get(), surface.get()), SDL_DestroyTexture};
			texture != nullptr)
		{
			buttonTexturesPS5.push_back(texture);
		}
		else
		{
			return std::make_unique<ConfigFailure>("IMG " + pathName + " Texture Creating Error", IMG_GetError());
		}
	}
	{
		std::string id = "Images.PS5_Home";
		const auto pathName(gameConfig.pTreeIni.get<std::string>(id, id + " path from config.ini"));
		std::shared_ptr<SDL_Surface> surface{nullptr};
		if (surface = {IMG_Load(pathName.c_str()), SDL_FreeSurface};
			surface == nullptr)
		{
			return std::make_unique<ConfigFailure>("IMG " + pathName + " Loading Error", IMG_GetError());
		}

		std::shared_ptr<SDL_Texture> texture{nullptr};
		if (texture = {SDL_CreateTextureFromSurface(renderer.get(), surface.get()), SDL_DestroyTexture};
			texture != nullptr)
		{
			buttonTexturesPS5.push_back(texture);
		}
		else
		{
			return std::make_unique<ConfigFailure>("IMG " + pathName + " Texture Creating Error", IMG_GetError());
		}
	}
	{
		std::string id = "Images.PS5_Options";
		const auto pathName(gameConfig.pTreeIni.get<std::string>(id, id + " path from config.ini"));
		std::shared_ptr<SDL_Surface> surface{nullptr};
		if (surface = {IMG_Load(pathName.c_str()), SDL_FreeSurface};
			surface == nullptr)
		{
			return std::make_unique<ConfigFailure>("IMG " + pathName + " Loading Error", IMG_GetError());
		}

		std::shared_ptr<SDL_Texture> texture{nullptr};
		if (texture = {SDL_CreateTextureFromSurface(renderer.get(), surface.get()), SDL_DestroyTexture};
			texture != nullptr)
		{
			buttonTexturesPS5.push_back(texture);
		}
		else
		{
			return std::make_unique<ConfigFailure>("IMG " + pathName + " Texture Creating Error", IMG_GetError());
		}
	}
	{
		std::string id = "Images.PS5_Triangle";
		const auto pathName(gameConfig.pTreeIni.get<std::string>(id, id + " path from config.ini"));
		std::shared_ptr<SDL_Surface> surface{nullptr};
		if (surface = {IMG_Load(pathName.c_str()), SDL_FreeSurface};
			surface == nullptr)
		{
			return std::make_unique<ConfigFailure>("IMG " + pathName + " Loading Error", IMG_GetError());
		}

		std::shared_ptr<SDL_Texture> texture{nullptr};
		if (texture = {SDL_CreateTextureFromSurface(renderer.get(), surface.get()), SDL_DestroyTexture};
			texture != nullptr)
		{
			buttonTexturesPS5.push_back(texture);
		}
		else
		{
			return std::make_unique<ConfigFailure>("IMG " + pathName + " Texture Creating Error", IMG_GetError());
		}
	}

	// texture XBox controls hint loading
	{
		std::string id = "Images.XBox_D-Pad";
		const auto pathName(gameConfig.pTreeIni.get<std::string>(id, id + " path from config.ini"));
		std::shared_ptr<SDL_Surface> surface{nullptr};
		if (surface = {IMG_Load(pathName.c_str()), SDL_FreeSurface};
			surface == nullptr)
		{
			return std::make_unique<ConfigFailure>("IMG " + pathName + " Loading Error", IMG_GetError());
		}

		std::shared_ptr<SDL_Texture> texture{nullptr};
		if (texture = {SDL_CreateTextureFromSurface(renderer.get(), surface.get()), SDL_DestroyTexture};
			texture != nullptr)
		{
			buttonTexturesXBox.push_back(texture);
		}
		else
		{
			return std::make_unique<ConfigFailure>("IMG " + pathName + " Texture Creating Error", IMG_GetError());
		}
	}
	{
		std::string id = "Images.XBox_Home";
		const auto pathName(gameConfig.pTreeIni.get<std::string>(id, id + " path from config.ini"));
		std::shared_ptr<SDL_Surface> surface{nullptr};
		if (surface = {IMG_Load(pathName.c_str()), SDL_FreeSurface};
			surface == nullptr)
		{
			return std::make_unique<ConfigFailure>("IMG " + pathName + " Loading Error", IMG_GetError());
		}

		std::shared_ptr<SDL_Texture> texture{nullptr};
		if (texture = {SDL_CreateTextureFromSurface(renderer.get(), surface.get()), SDL_DestroyTexture};
			texture != nullptr)
		{
			buttonTexturesXBox.push_back(texture);
		}
		else
		{
			return std::make_unique<ConfigFailure>("IMG " + pathName + " Texture Creating Error", IMG_GetError());
		}
	}
	{
		std::string id = "Images.XBox_Menu";
		const auto pathName(gameConfig.pTreeIni.get<std::string>(id, id + " path from config.ini"));
		std::shared_ptr<SDL_Surface> surface{nullptr};
		if (surface = {IMG_Load(pathName.c_str()), SDL_FreeSurface};
			surface == nullptr)
		{
			return std::make_unique<ConfigFailure>("IMG " + pathName + " Loading Error", IMG_GetError());
		}

		std::shared_ptr<SDL_Texture> texture{nullptr};
		if (texture = {SDL_CreateTextureFromSurface(renderer.get(), surface.get()), SDL_DestroyTexture};
			texture != nullptr)
		{
			buttonTexturesXBox.push_back(texture);
		}
		else
		{
			return std::make_unique<ConfigFailure>("IMG " + pathName + " Texture Creating Error", IMG_GetError());
		}
	}
	{
		std::string id = "Images.XBox_View";
		const auto pathName(gameConfig.pTreeIni.get<std::string>(id, id + " path from config.ini"));
		std::shared_ptr<SDL_Surface> surface{nullptr};
		if (surface = {IMG_Load(pathName.c_str()), SDL_FreeSurface};
			surface == nullptr)
		{
			return std::make_unique<ConfigFailure>("IMG " + pathName + " Loading Error", IMG_GetError());
		}

		std::shared_ptr<SDL_Texture> texture{nullptr};
		if (texture = {SDL_CreateTextureFromSurface(renderer.get(), surface.get()), SDL_DestroyTexture};
			texture != nullptr)
		{
			buttonTexturesXBox.push_back(texture);
		}
		else
		{
			return std::make_unique<ConfigFailure>("IMG " + pathName + " Texture Creating Error", IMG_GetError());
		}
	}
	{
		std::string id = "Images.XBox_A";
		const auto pathName(gameConfig.pTreeIni.get<std::string>(id, id + " path from config.ini"));
		std::shared_ptr<SDL_Surface> surface{nullptr};
		if (surface = {IMG_Load(pathName.c_str()), SDL_FreeSurface};
			surface == nullptr)
		{
			return std::make_unique<ConfigFailure>("IMG " + pathName + " Loading Error", IMG_GetError());
		}

		std::shared_ptr<SDL_Texture> texture{nullptr};
		if (texture = {SDL_CreateTextureFromSurface(renderer.get(), surface.get()), SDL_DestroyTexture};
			texture != nullptr)
		{
			buttonTexturesXBox.push_back(texture);
		}
		else
		{
			return std::make_unique<ConfigFailure>("IMG " + pathName + " Texture Creating Error", IMG_GetError());
		}
	}
	{
		std::string id = "Images.XBox_Y";
		const auto pathName(gameConfig.pTreeIni.get<std::string>(id, id + " path from config.ini"));
		std::shared_ptr<SDL_Surface> surface{nullptr};
		if (surface = {IMG_Load(pathName.c_str()), SDL_FreeSurface};
			surface == nullptr)
		{
			return std::make_unique<ConfigFailure>("IMG " + pathName + " Loading Error", IMG_GetError());
		}

		std::shared_ptr<SDL_Texture> texture{nullptr};
		if (texture = {SDL_CreateTextureFromSurface(renderer.get(), surface.get()), SDL_DestroyTexture};
			texture != nullptr)
		{
			buttonTexturesXBox.push_back(texture);
		}
		else
		{
			return std::make_unique<ConfigFailure>("IMG " + pathName + " Texture Creating Error", IMG_GetError());
		}
	}

	// texture atlas loading
	std::shared_ptr<SDL_Texture> atlasTexture{nullptr};
	{
		std::shared_ptr<SDL_Surface> atlasSurface{nullptr};
		const auto textureAtlasPath(
				gameConfig.pTreeIni.get<std::string>("Images.SpriteSheet", "Images.SpriteSheet path from config.ini"));
		if (atlasSurface = {IMG_Load(textureAtlasPath.c_str()), SDL_FreeSurface};
			atlasSurface == nullptr)
		{
			return std::make_unique<ConfigFailure>("IMG atlas Surface Loading Error", IMG_GetError());
		}

		const auto rawSurface = atlasSurface.get();
		if (const int result = SDL_SetColorKey(rawSurface, SDL_TRUE, SDL_MapRGB(rawSurface->format, 0, 0, 1));
			result != 0)
		{
			return std::make_unique<ConfigFailure>("IMG atlas SetColorKey Error", SDL_GetError());
		}

		if (atlasTexture = {SDL_CreateTextureFromSurface(renderer.get(), rawSurface), SDL_DestroyTexture};
			atlasTexture == nullptr)
		{
			return std::make_unique<ConfigFailure>("IMG atlas Texture Creating Error", IMG_GetError());
		}

		SDL_SetTextureBlendMode(atlasTexture.get(), SDL_BLENDMODE_BLEND);
	}

	// Audio loading and play
	{
		if (const int audioResult = Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048);
			audioResult >= 0)
		{
			const auto introMusicPathName(
					gameConfig.pTreeIni.get<std::string>("Music.LevelStarted",
														 "Music.LevelStarted path from config.ini"));
			if (levelStartedSound = {Mix_LoadWAV(introMusicPathName.c_str()), Mix_FreeChunk};
				levelStartedSound != nullptr)
			{
				if (const int playResult = Mix_PlayChannel(-1, levelStartedSound.get(), 0);
					playResult == -1)
				{
					std::cout << "Mix_PlayChannel, can't play levelStarted.wav, sound off, " << Mix_GetError() << '\n';
				}
			}
			else
			{
				std::cout << "Mix_LoadWAV, can't load levelStarted.wav, sound off, " << Mix_GetError() << '\n';
			}
		}
		else
		{
			std::cout << "Mix_OpenAudio, can't initialize sound card, sound off, " << Mix_GetError() << '\n';
		}
	}

	return std::make_unique<ConfigSuccess>(windowSize, gameConfig, buttonTexturesPS5, buttonTexturesXBox, renderer,
										   fontSmall, fontMedium, logoTexture, atlasTexture, joyIconTexture, isVsyncOn);
}

[[nodiscard]]
std::unique_ptr<SDL_Window, decltype(&SDL_DestroyWindow)> SDLEnvironment::InitWindow(UPoint& windowSizeHalf) const
{
	const auto title = "Battle City remastered";
	constexpr SDL_WindowFlags windowFlags = SDL_WINDOW_SHOWN;
	const SDL_Rect rect{.x = static_cast<int>(windowPos.x),
						.y = static_cast<int>(windowPos.y),
						.w = static_cast<int>(windowSize.x),
						.h = static_cast<int>(windowSize.y)};
	windowSizeHalf = {.x = static_cast<size_t>(rect.w / 2), .y = static_cast<size_t>(rect.h / 2)};

	return {SDL_CreateWindow(title, rect.x, rect.y, rect.w, rect.h, windowFlags), SDL_DestroyWindow};
}

[[nodiscard]] std::shared_ptr<SDL_Renderer> SDLEnvironment::InitRender(UPoint& windowSizeHalf) const
{
	Uint32 renderFlags = SDL_RENDERER_ACCELERATED;
	if (isVsyncOn)
	{
		renderFlags |= SDL_RENDERER_PRESENTVSYNC;
	}

	constexpr int monitorIndex = -1;//NOTE: -1 mean use the default//TODO: move to userSettings
	SDL_Rect bounds;
	SDL_GetDisplayBounds(monitorIndex, &bounds);//TODO: investigate errors: displayIndex must be in the range 0 - 1

	SDL_Rect bordersSize;
	SDL_GetWindowBordersSize(sdlWindow.get(), &bordersSize.y, &bordersSize.x, &bordersSize.h, &bordersSize.w);

	if constexpr (monitorIndex != -1)
	{
		SDL_SetWindowPosition(sdlWindow.get(),
							  static_cast<int>(bounds.x + bounds.w / 2 - windowSizeHalf.x / 2),
							  static_cast<int>(bounds.y + bounds.h / 2 - windowSizeHalf.y / 2 - bordersSize.y));
	}

	return {SDL_CreateRenderer(sdlWindow.get(), monitorIndex, renderFlags), SDL_DestroyRenderer};
}
