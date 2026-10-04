#pragma once

#include "utils/Log.h"
#include <cstddef>
#include <exception>
#include <expected>
#include <filesystem>
#include <optional>
#include <string>
#include <boost/property_tree/ptree.hpp>

//NOTE: line 0 means the file could not be opened, and only such a file is rewritten with the defaults
struct ConfigError final
{
	std::filesystem::path path{};
	std::string reason{};
	std::size_t line{};
};

//NOTE: startup input - the ini store and asset paths, fixed once the game runs. Kept apart from GameConfig
//so property_tree stays out of everything that includes that
class ProjectConfig final
{
public:
	explicit ProjectConfig(std::filesystem::path filePath, bool skipIni = false);

	~ProjectConfig();

	//NOTE: next to the exe - each build tree keeps its own settings, like its own binaries
	[[nodiscard]] static std::filesystem::path DefaultFilePath();

	//NOTE: only a file that exists and does not parse lands here; a missing one is written, not reported
	[[nodiscard]] const std::optional<ConfigError>& LoadError() const noexcept { return _loadError; }

	//NOTE: the default doubles as the message SDL prints when the key is missing
	[[nodiscard]] std::filesystem::path ResourcePath(const std::string& key) const;

	//NOTE: -1 is adaptive, n waits n refreshes, 0 is off; a true/false value does not parse and reads as 0
	[[nodiscard]] int VSyncMode() const { return Get<int>("Window.vsync", 0); }
	[[nodiscard]] bool IsVsyncOn() const { return VSyncMode() != 0; }
	[[nodiscard]] int MonitorNumber() const { return Get<int>("Window.MonitorNumber", 1); }
	[[nodiscard]] bool IsCenterOnStart() const { return Get<bool>("Window.centerOnStart", false); }
	//NOTE: out of 32767 - a worn stick rests further from the center
	[[nodiscard]] int GamepadDeadZone() const { return Get<int>("Gamepad.deadZone", 8000); }
	//NOTE: the host this game last joined as a client - empty until it joins one
	[[nodiscard]] std::string LastConnectAddress() const { return Get<std::string>("Network.LastConnectAddress", {}); }
	void SetLastConnectAddress(const std::string& host) { Set("Network.LastConnectAddress", host); }
	[[nodiscard]] bool IsFreshIni() const noexcept { return _isFreshIni; }

	//NOTE: the destructor writes the file too - this is for what must not wait for the game to close
	void Save() const;

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
	bool _skipIniLoad{};
	bool _isFreshIni{};
};
