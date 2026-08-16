#pragma once
#include "Point.h"
#include <iostream>
#include <memory>
#include <boost/property_tree/ptree.hpp>

struct LaunchOptions;

class GameConfig
{
public:
	explicit GameConfig(std::string filePath, bool skipIniLoad = false);
	~GameConfig();

	void LoadIni(const std::string& filePath);
	void DefaultInitIni();
	void SaveIni(const std::string& filePath) const;
	void Apply(const LaunchOptions& launchOptions);
	void ApplyWindowOffsetAsHost();
	void ApplyWindowOffsetAsClient();

	UPoint windowSize{};
	UPoint windowPos{};
	UPoint windowSizeDefault{.x = 800u, .y = 600u};
	UPoint windowsPosOffset{};
	size_t sideBarWidth{175u};
	int tankHealth{100};
	float scaleFactor{1.f};
	float defaultScaleFactor{1.f};
	float gridSize{50.f};
	float gridSizeDefault{50.f};
	float gridOffset{600 / gridSize};
	float tankSize{gridOffset * 3};
	float tankSpeed{142};
	float tankSpeedDefault{142};
	int bonusSize{static_cast<int>(gridOffset * 3)};
	bool skipIniLoad{false};
	bool skipIntroMusic{false};//NOTE: launch flag, not persisted - autoplay only, sound stays on
	bool hasExplicitWindowPos{false};//NOTE: explicit pos wins over monitor centering

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
	std::string _filePath;
};
