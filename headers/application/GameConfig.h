#pragma once
#include "Point.h"
#include <expected>
#include <iostream>
#include <memory>
#include <optional>
#include <boost/property_tree/ptree.hpp>

struct LaunchOptions;
struct WorldGeometry;

//NOTE: shaped like MapError. line 0 means the file as a whole, and that is what decides
//whether it gets rewritten - see the constructor.
struct ConfigError final
{
	std::string path{};
	std::string reason{};
	std::size_t line{};
};

class GameConfig
{
public:
	explicit GameConfig(std::string filePath, bool skipIni = false);
	~GameConfig();

	[[nodiscard]] std::expected<void, ConfigError> LoadIni(const std::string& filePath);
	void DefaultInitIni();
	void SaveIni(const std::string& filePath) const;
	void Apply(const LaunchOptions& launchOptions);
	//NOTE: the one place where a cell size turns into every other size - keep derivations out of the spawners
	void ApplyGeometry(const WorldGeometry& geometry, std::size_t mapRows);
	void ApplyWindowOffsetAsHost();
	void ApplyWindowOffsetAsClient();

	UPoint windowSize{};
	UPoint windowPos{};
	UPoint windowSizeDefault{.x = 800u, .y = 600u};
	UPoint windowsPosOffset{};
	size_t sideBarWidth{175u};
	int tankHealth{100};
	float scaleFactor{1.f};
	//NOTE: gridSize is a count - how many cells fit vertically; gridOffset is one cell in pixels
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

	//NOTE: only a file that exists and does not parse lands here; a missing one is written, not reported
	[[nodiscard]] const std::optional<ConfigError>& LoadError() const
	{
		return _loadError;
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
	std::optional<ConfigError> _loadError{};
};
