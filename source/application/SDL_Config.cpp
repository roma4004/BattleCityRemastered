#include "application/SDL_Config.h"
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

SDL_Config::SDL_Config(GameConfig& gameConfig)
	: gameConfig{gameConfig} {}

SDL_Config::~SDL_Config()
{
	Mix_CloseAudio();
	fontSmall.reset();
	fontMedium.reset();
	TTF_Quit();
	IMG_Quit();
	SDL_Quit();
}

std::unique_ptr<IConfig> SDL_Config::Init()
{
	if (SDL_Init(SDL_INIT_EVERYTHING) != 0)
	{
		return std::make_unique<ConfigFailure>("SDL_Init Error: ", SDL_GetError());
	}

	// creating window
	if (sdlWindow = InitWindow();
		sdlWindow == nullptr)
	{
		return std::make_unique<ConfigFailure>("SDL_CreateWindow Error", SDL_GetError());
	}

	// creating renderer
	if (renderer = InitRender();
		renderer == nullptr)
	{
		return std::make_unique<ConfigFailure>("SDL_CreateRenderer Error", SDL_GetError());
	}

	SDL_SetRenderDrawBlendMode(renderer.get(), SDL_BLENDMODE_BLEND);

	// font init and loading
	{
		const auto fontPathName(
				gameConfig.Get<std::string>("Fonts.BattleCity", "Fonts.BattleCity path from config.ini"));
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

	//TODO: split surface loading and texture creation to recreate all texture if vsync change
	// texture logo loading
	{
		const auto logoPathName(
				gameConfig.Get<std::string>("Images.Logo", "Images.Logo path from config.ini"));
		if (logoSurface = {IMG_Load(logoPathName.c_str()), SDL_FreeSurface};
			logoSurface == nullptr)//TODO: Store all surface to recreate all texture if vsync change
		{
			return std::make_unique<ConfigFailure>("IMG Logo Loading Error", IMG_GetError());
		}

		if (logoTexture = {
					SDL_CreateTextureFromSurface(renderer.get(), logoSurface.get()),
					SDL_DestroyTexture};
			logoTexture == nullptr)
		{
			return std::make_unique<ConfigFailure>("IMG Logo Texture Creating Error", IMG_GetError());
		}
	}

	// texture selector icon loading
	{
		const auto selectorIconPathName(
				gameConfig.Get<std::string>("Images.MenuSelectorP1",
											"Images.MenuSelectorP1 path from config.ini"));
		if (selectorIconSurface = {IMG_Load(selectorIconPathName.c_str()), SDL_FreeSurface};
			selectorIconSurface == nullptr)
		{
			return std::make_unique<ConfigFailure>("IMG Selector Icon Loading Error", IMG_GetError());
		}

		if (selectorIconTexture = {
					SDL_CreateTextureFromSurface(renderer.get(), selectorIconSurface.get()),
					SDL_DestroyTexture};
			selectorIconTexture == nullptr)
		{
			return std::make_unique<ConfigFailure>("IMG Selector Ion Texture Creating Error", IMG_GetError());
		}
	}

	// texture PS5 controls hint loading
	{
		std::string id = "Images.PS5_Create";
		const auto pathName(gameConfig.Get<std::string>(id, id + " path from config.ini"));
		std::shared_ptr<SDL_Surface> surface{nullptr};
		if (surface = {IMG_Load(pathName.c_str()), SDL_FreeSurface};
			surface == nullptr)
		{
			return std::make_unique<ConfigFailure>("IMG " + pathName + " Loading Error", IMG_GetError());
		}

		surfacePS5.push_back(surface);

		std::shared_ptr<SDL_Texture> texture{nullptr};
		if (texture = {SDL_CreateTextureFromSurface(renderer.get(), surface.get()), SDL_DestroyTexture};
			texture != nullptr)
		{
			ps5Textures.push_back(texture);
		}
		else
		{
			return std::make_unique<ConfigFailure>("IMG " + pathName + " Texture Creating Error", IMG_GetError());
		}
	}
	{
		std::string id = "Images.PS5_Cross";
		const auto pathName(gameConfig.Get<std::string>(id, id + " path from config.ini"));
		std::shared_ptr<SDL_Surface> surface{nullptr};
		if (surface = {IMG_Load(pathName.c_str()), SDL_FreeSurface};
			surface == nullptr)
		{
			return std::make_unique<ConfigFailure>("IMG " + pathName + " Loading Error", IMG_GetError());
		}

		surfacePS5.push_back(surface);

		std::shared_ptr<SDL_Texture> texture{nullptr};
		if (texture = {SDL_CreateTextureFromSurface(renderer.get(), surface.get()), SDL_DestroyTexture};
			texture != nullptr)
		{
			ps5Textures.push_back(texture);
		}
		else
		{
			return std::make_unique<ConfigFailure>("IMG " + pathName + " Texture Creating Error", IMG_GetError());
		}
	}
	{
		std::string id = "Images.PS5_D-Pad";
		const auto pathName(gameConfig.Get<std::string>(id, id + " path from config.ini"));
		std::shared_ptr<SDL_Surface> surface{nullptr};
		if (surface = {IMG_Load(pathName.c_str()), SDL_FreeSurface};
			surface == nullptr)
		{
			return std::make_unique<ConfigFailure>("IMG " + pathName + " Loading Error", IMG_GetError());
		}

		surfacePS5.push_back(surface);

		std::shared_ptr<SDL_Texture> texture{nullptr};
		if (texture = {SDL_CreateTextureFromSurface(renderer.get(), surface.get()), SDL_DestroyTexture};
			texture != nullptr)
		{
			ps5Textures.push_back(texture);
		}
		else
		{
			return std::make_unique<ConfigFailure>("IMG " + pathName + " Texture Creating Error", IMG_GetError());
		}
	}
	{
		std::string id = "Images.PS5_Home";
		const auto pathName(gameConfig.Get<std::string>(id, id + " path from config.ini"));
		std::shared_ptr<SDL_Surface> surface{nullptr};
		if (surface = {IMG_Load(pathName.c_str()), SDL_FreeSurface};
			surface == nullptr)
		{
			return std::make_unique<ConfigFailure>("IMG " + pathName + " Loading Error", IMG_GetError());
		}

		surfacePS5.push_back(surface);

		std::shared_ptr<SDL_Texture> texture{nullptr};
		if (texture = {SDL_CreateTextureFromSurface(renderer.get(), surface.get()), SDL_DestroyTexture};
			texture != nullptr)
		{
			ps5Textures.push_back(texture);
		}
		else
		{
			return std::make_unique<ConfigFailure>("IMG " + pathName + " Texture Creating Error", IMG_GetError());
		}
	}
	{
		std::string id = "Images.PS5_Options";
		const auto pathName(gameConfig.Get<std::string>(id, id + " path from config.ini"));
		std::shared_ptr<SDL_Surface> surface{nullptr};
		if (surface = {IMG_Load(pathName.c_str()), SDL_FreeSurface};
			surface == nullptr)
		{
			return std::make_unique<ConfigFailure>("IMG " + pathName + " Loading Error", IMG_GetError());
		}

		surfacePS5.push_back(surface);

		std::shared_ptr<SDL_Texture> texture{nullptr};
		if (texture = {SDL_CreateTextureFromSurface(renderer.get(), surface.get()), SDL_DestroyTexture};
			texture != nullptr)
		{
			ps5Textures.push_back(texture);
		}
		else
		{
			return std::make_unique<ConfigFailure>("IMG " + pathName + " Texture Creating Error", IMG_GetError());
		}
	}
	{
		std::string id = "Images.PS5_Triangle";
		const auto pathName(gameConfig.Get<std::string>(id, id + " path from config.ini"));
		std::shared_ptr<SDL_Surface> surface{nullptr};
		if (surface = {IMG_Load(pathName.c_str()), SDL_FreeSurface};
			surface == nullptr)
		{
			return std::make_unique<ConfigFailure>("IMG " + pathName + " Loading Error", IMG_GetError());
		}

		surfacePS5.push_back(surface);

		std::shared_ptr<SDL_Texture> texture{nullptr};
		if (texture = {SDL_CreateTextureFromSurface(renderer.get(), surface.get()), SDL_DestroyTexture};
			texture != nullptr)
		{
			ps5Textures.push_back(texture);
		}
		else
		{
			return std::make_unique<ConfigFailure>("IMG " + pathName + " Texture Creating Error", IMG_GetError());
		}
	}

	// texture XBox controls hint loading
	{
		std::string id = "Images.XBox_D-Pad";
		const auto pathName(gameConfig.Get<std::string>(id, id + " path from config.ini"));
		std::shared_ptr<SDL_Surface> surface{nullptr};
		if (surface = {IMG_Load(pathName.c_str()), SDL_FreeSurface};
			surface == nullptr)
		{
			return std::make_unique<ConfigFailure>("IMG " + pathName + " Loading Error", IMG_GetError());
		}

		surfaceXBox.push_back(surface);

		std::shared_ptr<SDL_Texture> texture{nullptr};
		if (texture = {SDL_CreateTextureFromSurface(renderer.get(), surface.get()), SDL_DestroyTexture};
			texture != nullptr)
		{
			xboxTextures.push_back(texture);
		}
		else
		{
			return std::make_unique<ConfigFailure>("IMG " + pathName + " Texture Creating Error", IMG_GetError());
		}
	}
	{
		std::string id = "Images.XBox_Home";
		const auto pathName(gameConfig.Get<std::string>(id, id + " path from config.ini"));
		std::shared_ptr<SDL_Surface> surface{nullptr};
		if (surface = {IMG_Load(pathName.c_str()), SDL_FreeSurface};
			surface == nullptr)
		{
			return std::make_unique<ConfigFailure>("IMG " + pathName + " Loading Error", IMG_GetError());
		}

		surfaceXBox.push_back(surface);

		std::shared_ptr<SDL_Texture> texture{nullptr};
		if (texture = {SDL_CreateTextureFromSurface(renderer.get(), surface.get()), SDL_DestroyTexture};
			texture != nullptr)
		{
			xboxTextures.push_back(texture);
		}
		else
		{
			return std::make_unique<ConfigFailure>("IMG " + pathName + " Texture Creating Error", IMG_GetError());
		}
	}
	{
		std::string id = "Images.XBox_Menu";
		const auto pathName(gameConfig.Get<std::string>(id, id + " path from config.ini"));
		std::shared_ptr<SDL_Surface> surface{nullptr};
		if (surface = {IMG_Load(pathName.c_str()), SDL_FreeSurface};
			surface == nullptr)
		{
			return std::make_unique<ConfigFailure>("IMG " + pathName + " Loading Error", IMG_GetError());
		}

		surfaceXBox.push_back(surface);

		std::shared_ptr<SDL_Texture> texture{nullptr};
		if (texture = {SDL_CreateTextureFromSurface(renderer.get(), surface.get()), SDL_DestroyTexture};
			texture != nullptr)
		{
			xboxTextures.push_back(texture);
		}
		else
		{
			return std::make_unique<ConfigFailure>("IMG " + pathName + " Texture Creating Error", IMG_GetError());
		}
	}
	{
		std::string id = "Images.XBox_View";
		const auto pathName(gameConfig.Get<std::string>(id, id + " path from config.ini"));
		std::shared_ptr<SDL_Surface> surface{nullptr};
		if (surface = {IMG_Load(pathName.c_str()), SDL_FreeSurface};
			surface == nullptr)
		{
			return std::make_unique<ConfigFailure>("IMG " + pathName + " Loading Error", IMG_GetError());
		}

		surfaceXBox.push_back(surface);

		std::shared_ptr<SDL_Texture> texture{nullptr};
		if (texture = {SDL_CreateTextureFromSurface(renderer.get(), surface.get()), SDL_DestroyTexture};
			texture != nullptr)
		{
			xboxTextures.push_back(texture);
		}
		else
		{
			return std::make_unique<ConfigFailure>("IMG " + pathName + " Texture Creating Error", IMG_GetError());
		}
	}
	{
		std::string id = "Images.XBox_A";
		const auto pathName(gameConfig.Get<std::string>(id, id + " path from config.ini"));
		std::shared_ptr<SDL_Surface> surface{nullptr};
		if (surface = {IMG_Load(pathName.c_str()), SDL_FreeSurface};
			surface == nullptr)
		{
			return std::make_unique<ConfigFailure>("IMG " + pathName + " Loading Error", IMG_GetError());
		}

		surfaceXBox.push_back(surface);

		std::shared_ptr<SDL_Texture> texture{nullptr};
		if (texture = {SDL_CreateTextureFromSurface(renderer.get(), surface.get()), SDL_DestroyTexture};
			texture != nullptr)
		{
			xboxTextures.push_back(texture);
		}
		else
		{
			return std::make_unique<ConfigFailure>("IMG " + pathName + " Texture Creating Error", IMG_GetError());
		}
	}
	{
		std::string id = "Images.XBox_Y";
		const auto pathName(gameConfig.Get<std::string>(id, id + " path from config.ini"));
		std::shared_ptr<SDL_Surface> surface{nullptr};
		if (surface = {IMG_Load(pathName.c_str()), SDL_FreeSurface};
			surface == nullptr)
		{
			return std::make_unique<ConfigFailure>("IMG " + pathName + " Loading Error", IMG_GetError());
		}

		surfaceXBox.push_back(surface);

		std::shared_ptr<SDL_Texture> texture{nullptr};
		if (texture = {SDL_CreateTextureFromSurface(renderer.get(), surface.get()), SDL_DestroyTexture};
			texture != nullptr)
		{
			xboxTextures.push_back(texture);
		}
		else
		{
			return std::make_unique<ConfigFailure>("IMG " + pathName + " Texture Creating Error", IMG_GetError());
		}
	}

	// texture atlas loading
	{
		const auto textureAtlasPath(
				gameConfig.Get<std::string>("Images.SpriteSheet", "Images.SpriteSheet path from config.ini"));
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

		if (atlasTexture = {SDL_CreateTextureFromSurface(renderer.get(), rawSurface),
							SDL_DestroyTexture};
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
					gameConfig.Get<std::string>("Music.LevelStarted", "Music.LevelStarted path from config.ini"));
			if (levelIntroMusic = {Mix_LoadWAV(introMusicPathName.c_str()), Mix_FreeChunk};
				levelIntroMusic != nullptr)
			{
				//TODO: move to soundManager
				//NOTE: autoplay only - device and chunk stay ready
				if (!gameConfig.skipIntroMusic)
				{
					if (const int playResult = Mix_PlayChannel(-1, levelIntroMusic.get(), 0); playResult == -1)
					{
						std::cout << "Mix_PlayChannel, can't play levelStarted.wav, sound off, " << Mix_GetError()
								  << '\n';
					}
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

	return std::make_unique<ConfigSuccess>(gameConfig);
}

std::unique_ptr<SDL_Window, decltype(&SDL_DestroyWindow)> SDL_Config::InitWindow() const
{
	constexpr auto title = "Battle City remastered";
	constexpr SDL_WindowFlags windowFlags = SDL_WINDOW_RESIZABLE;
	const SDL_Rect rect{.x = static_cast<int>(gameConfig.windowPos.x),
						.y = static_cast<int>(gameConfig.windowPos.y),
						.w = static_cast<int>(gameConfig.windowSize.x),
						.h = static_cast<int>(gameConfig.windowSize.y)};

	return {SDL_CreateWindow(title, rect.x, rect.y, rect.w, rect.h, windowFlags), SDL_DestroyWindow};
}

std::shared_ptr<SDL_Renderer> SDL_Config::InitRender() const
{
	Uint32 renderFlags = SDL_RENDERER_ACCELERATED;
	if (const bool vsync = gameConfig.Get<bool>("Window.vsync", false))
	{
		renderFlags |= SDL_RENDERER_PRESENTVSYNC;//TODO: recreate render if vsync change
	}

	const int monitorIndex = gameConfig.Get<int>("Window.MonitorNumber", 1) - 1;
	SDL_Rect bounds;
	SDL_GetDisplayBounds(monitorIndex, &bounds);

	SDL_Rect bordersSize;
	SDL_Window* sdlWindowRaw = sdlWindow.get();
	SDL_GetWindowBordersSize(sdlWindowRaw, &bordersSize.y, &bordersSize.x, &bordersSize.h, &bordersSize.w);

	//NOTE: centering would override an explicit pos
	if (monitorIndex != -1 && !gameConfig.hasExplicitWindowPos)
	{
		const Point screenCenter{.x = bounds.x + bounds.w / 2,
								 .y = bounds.y + bounds.h / 2};
		const Point windowHalfSize{.x = static_cast<int>(gameConfig.windowSize.x) / 2,
								   .y = static_cast<int>(gameConfig.windowSize.y) / 2};
		SDL_SetWindowPosition(sdlWindowRaw,
							  screenCenter.x - windowHalfSize.x + static_cast<int>(gameConfig.windowsPosOffset.x),
							  screenCenter.y - windowHalfSize.y + static_cast<int>(gameConfig.windowsPosOffset.y)
							  - bordersSize.y);
	}

	return {SDL_CreateRenderer(sdlWindowRaw, monitorIndex, renderFlags), SDL_DestroyRenderer};
}
