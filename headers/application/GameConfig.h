#pragma once
#include "Point.h"
#include <iostream>
#include <memory>
#include <vector>
#include <boost/property_tree/ptree.hpp>

class GameConfig
{
public:
	explicit GameConfig(std::string filePath);
	~GameConfig();

	void LoadIni(const std::string& filePath);
	void DefaultInitIni(const std::string& filePath);
	void SaveIni(const std::string& filePath) const;
	void ApplyWindowOffsetAsHost();
	void ApplyWindowOffsetAsClient();

	UPoint windowSize{};
	UPoint windowPos{};
	UPoint windowSizeDefault{800, 600};
	UPoint windowsPosOffset{};
	size_t sideBarWidth{175u};
	float scaleFactor{1.f};
	float previousScaleFactor{1.f};
	float gridSize{50.f};
	float gridSizeDefault{50.f};
	float gridOffset{600 / gridSize};
	float tankSize{gridOffset * 3};
	int bonusSize{static_cast<int>(gridOffset * 3)};
	float tankSpeed{142};
	float tankSpeedDefault{142};
	int tankHealth{100};

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
