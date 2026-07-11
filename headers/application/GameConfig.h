#pragma once
#include "Point.h"
#include <SDL_mixer.h>
#include <SDL_render.h>
#include <SDL_ttf.h>
#include <iostream>
#include <memory>
#include <vector>
#include <boost/property_tree/ptree.hpp> //TODO: update cmake and nuget boost to boost-1.91.0-1

class GameConfig
{
public:
	explicit GameConfig(const std::string& filePath);
	~GameConfig();

	void LoadIni(const std::string& filePath);
	void DefaultInitIni(const std::string& filePath);
	void SaveIni(const std::string& filePath) const;
	void ApplyWindowOffsetAsHost();
	void ApplyWindowOffsetAsClient();

	UPoint windowSize{};
	UPoint windowPos{};
	UPoint windowsPosOffset{};

	std::unique_ptr<SDL_Window, decltype(&SDL_DestroyWindow)> sdlWindow{nullptr, nullptr};
	std::shared_ptr<SDL_Renderer> renderer{nullptr};
	std::shared_ptr<TTF_Font> fontSmall{nullptr};
	std::shared_ptr<TTF_Font> fontMedium{nullptr};
	std::unique_ptr<Mix_Chunk, decltype(&Mix_FreeChunk)> levelStartedSound{nullptr, nullptr}; //TODO: soundManager

	std::shared_ptr<SDL_Texture> logoTexture{nullptr};
	std::shared_ptr<SDL_Texture> atlasTexture{nullptr};
	std::shared_ptr<SDL_Texture> selectorIconTexture{nullptr};
	std::vector<std::shared_ptr<SDL_Texture>> buttonTexturesPS5;
	std::vector<std::shared_ptr<SDL_Texture>> buttonTexturesXBox;

	std::shared_ptr<SDL_Surface> logoSurface{nullptr};
	std::shared_ptr<SDL_Surface> atlasSurface{nullptr};
	std::shared_ptr<SDL_Surface> selectorIconSurface{nullptr};
	std::vector<std::shared_ptr<SDL_Surface>> buttonSurfacePS5;
	std::vector<std::shared_ptr<SDL_Surface>> buttonSurfaceXBox;

	template<typename T>
	[[nodiscard]] T Get(const std::string& key, const T& defaultValue) const
	{
		try
		{
			return _pTreeIni.get<T>(key, defaultValue);
		}
		catch (const std::exception&)
		{
			return defaultValue;
		}
	}

	template<typename T>
	void Set(const std::string& key, const T& value)
	{
		try
		{
			_pTreeIni.put<T>(key, value);
		}
		catch (const std::exception& err)
		{
			std::cerr << "Error setting config value for key '" << key << "': " << err.what() << '\n';
		}
	}

	[[nodiscard]] const boost::property_tree::ptree& GetTree() const
	{
		return _pTreeIni;
	}

	[[nodiscard]] boost::property_tree::ptree& GetTree()
	{
		return _pTreeIni;
	}

private:
	boost::property_tree::ptree _pTreeIni;
};
