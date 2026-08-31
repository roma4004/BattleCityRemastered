#pragma once

#include "utils/Log.h"
#include <cstddef>
#include <exception>
#include <expected>
#include <filesystem>
#include <optional>
#include <string>
#include <boost/property_tree/ptree.hpp>

//NOTE: shaped like MapError. line 0 means the whole file, and that decides whether it gets rewritten
struct ConfigError final
{
	std::filesystem::path path{};
	std::string reason{};
	std::size_t line{};
};

//NOTE: startup input - the ini store and where the assets live. Split from GameConfig because this
//half stops changing once the game runs, and to keep property_tree out of 40 unrelated files.
class ProjectConfig final
{
public:
	explicit ProjectConfig(std::filesystem::path filePath, bool skipIni = false);

	~ProjectConfig();

	//NOTE: next to the exe - each build tree keeps its own settings, like its own binaries
	[[nodiscard]] static std::filesystem::path DefaultFilePath();

	//NOTE: only a file that exists and does not parse lands here; a missing one is written, not reported
	[[nodiscard]] const std::optional<ConfigError>& LoadError() const { return _loadError; }

	//NOTE: the default doubles as the message SDL prints when the key is missing
	[[nodiscard]] std::filesystem::path ResourcePath(const std::string& key) const;

	//NOTE: SDL takes more than on/off - -1 is adaptive, n waits n refreshes, 0 is off. An older ini
	//holding true/false does not parse as an int and falls back to the default, which is off either way
	[[nodiscard]] int VSyncMode() const { return Get<int>("Window.vsync", 0); }
	[[nodiscard]] bool IsVsyncOn() const { return VSyncMode() != 0; }
	[[nodiscard]] int MonitorNumber() const { return Get<int>("Window.MonitorNumber", 1); }
	[[nodiscard]] bool IsCenterOnStart() const { return Get<bool>("Window.centerOnStart", false); }
	[[nodiscard]] bool IsFreshIni() const { return _isFreshIni; }

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
			Log::Error("cannot set config key '" + key + "': " + err.what());
		}
	}

private:
	[[nodiscard]] std::expected<void, ConfigError> LoadIni(const std::filesystem::path& filePath);
	void DefaultInitIni();
	void SaveIni(const std::filesystem::path& filePath) const;

	boost::property_tree::ptree _pTreeIni;
	std::filesystem::path _filePath;
	std::optional<ConfigError> _loadError{};
	bool _skipIniLoad{false};
	bool _isFreshIni{false};
};
