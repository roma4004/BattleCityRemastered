#include "application/ProjectConfig.h"
#include "network/Endpoints.h"
#include "utils/Log.h"
#include <exception>
#include <filesystem>
#include <optional>
#include <string>
#include <system_error>
#include <utility>
#include <boost/property_tree/ini_parser.hpp>

ProjectConfig::ProjectConfig(std::filesystem::path filePath, const bool skipIni)
	: _filePath(std::move(filePath))
	, _skipIniLoad{skipIni}
{
	if (_skipIniLoad)
	{
		DefaultInitIni();
		_isFreshIni = true;
		return;
	}

	if (const auto loaded{LoadIni(_filePath)}; !loaded)
	{
		DefaultInitIni();

		_isFreshIni = true;

		if (loaded.error().line == 0u)
		{
			SaveIni(_filePath);
		}
		else
		{
			_loadError = loaded.error();
		}
	}
}

ProjectConfig::~ProjectConfig() { Save(); }

//NOTE: _loadError means an unparseable file is there, and saving would overwrite it with the defaults
void ProjectConfig::Save() const
{
	if (!_skipIniLoad && !_loadError)
	{
		SaveIni(_filePath);
	}
}

std::filesystem::path ProjectConfig::DefaultFilePath()
{
	//NOTE: created here because SaveIni cannot make its own parent
	std::error_code errorCode{};
	std::filesystem::create_directories("Config", errorCode);

	return "Config/config.ini";
}

std::filesystem::path ProjectConfig::ResourcePath(const std::string& key) const
{
	return Get<std::string>(key, key + " path from config.ini");
}

std::optional<network::ServerAddress> ProjectConfig::LastServer(const bool isIPv6) const
{
	std::optional<network::ServerAddress> address{
			network::ParseServerAddress(Get<std::string>(isIPv6 ? "Network.LastIPv6" : "Network.LastIPv4", {}))};
	//NOTE: a hand-edited key of the other protocol would land in the wrong row
	if (address && address->host.contains(':') != isIPv6)
	{
		address.reset();
	}

	return address;
}

void ProjectConfig::SetLastServer(const network::ServerAddress& address)
{
	const bool isIPv6{address.host.contains(':')};
	Set(isIPv6 ? "Network.LastIPv6" : "Network.LastIPv4",
		(isIPv6 ? '[' + address.host + ']' : address.host) + ':' + std::to_string(address.port));
}

network::ServerAddress ProjectConfig::CatalogAddress(const bool isIPv6) const
{
	const std::string key{isIPv6 ? "Network.CatalogIPv6" : "Network.Catalog"};
	const std::string value{Get<std::string>(key, {})};
	network::ServerAddress address{.host = isIPv6 ? network::LocalIPv6Address() : network::LocalAddress()};
	if (const auto parsed{network::ParseServerAddress(value)}; parsed && parsed->host.contains(':') == isIPv6)
	{
		address = *parsed;
	}
	else if (!value.empty())
	{
		Log::Error("config " + key + " '" + value + "' is no " + (isIPv6 ? "IPv6" : "IPv4")
				   + " address, the catalog is asked on this machine");
	}

	if (address.port == network::kAnyFreePort)
	{
		address.port = network::kCatalogPort;
	}

	return address;
}

std::expected<void, ConfigError> ProjectConfig::LoadIni(const std::filesystem::path& filePath)
{
	try
	{
		boost::property_tree::ini_parser::read_ini(filePath.string(), _pTreeIni);
	}
	catch (const boost::property_tree::ini_parser_error& err)
	{
		//NOTE: boost reports line 0 for a file it could not open, a 1-based line for bad contents
		return std::unexpected(ConfigError{.path = filePath, .reason = err.message(), .line = err.line()});
	}

	//NOTE: a file can be older than a key - what it lacks takes the default, and the next save writes that in
	DefaultInitIni();

	return {};
}

void ProjectConfig::DefaultInitIni()
{
	const auto keyRepair = [this](const std::string& key, const auto& value)
	{
		if (!_pTreeIni.get_child_optional(key))
		{
			Set(key, value);
		}
	};

	try
	{
		keyRepair("Window.width", 800u);
		keyRepair("Window.height", 600u);
		keyRepair("Window.posX", 100u);
		keyRepair("Window.posY", 100u);
		keyRepair("Window.vsync", 0);
		keyRepair("Window.MonitorNumber", 1u);
		keyRepair("Window.centerOnStart", false);

		keyRepair("Sound.volume", 100u);
		keyRepair("Sound.onOff", true);

		keyRepair("Control.swap", false);
		keyRepair("Gamepad.deadZone", 8000);
		keyRepair("Network.LastIPv4", "");
		keyRepair("Network.LastIPv6", "");
		keyRepair("Network.Catalog", "");
		keyRepair("Network.CatalogIPv6", "");

		keyRepair("Fonts.BattleCity", "Resources/Fonts/PressStart2P-vaV7.ttf");
		keyRepair("Images.Logo", "Resources/Images/Title.png");
		keyRepair("Music.LevelStarted", "Resources/Sounds/levelStarted.wav");
		keyRepair("Images.SpriteSheet", "Resources/Images/SpriteSheet.png");
		keyRepair("Images.MenuSelectorP1", "Resources/Images/menuSelectorP1.png");

		keyRepair("Images.PS5_Create", "Resources/Images/PS5_Buttons/Create.png");
		keyRepair("Images.PS5_Cross", "Resources/Images/PS5_Buttons/Cross.png");
		keyRepair("Images.PS5_D-Pad", "Resources/Images/PS5_Buttons/D-Pad.png");
		keyRepair("Images.PS5_Home", "Resources/Images/PS5_Buttons/Home.png");
		keyRepair("Images.PS5_Options", "Resources/Images/PS5_Buttons/Options.png");
		keyRepair("Images.PS5_Square", "Resources/Images/PS5_Buttons/Square.png");
		keyRepair("Images.PS5_Triangle", "Resources/Images/PS5_Buttons/Triangle.png");

		keyRepair("Images.XBox_D-Pad", "Resources/Images/Xbox_Buttons/D-Pad.png");
		keyRepair("Images.XBox_Home", "Resources/Images/Xbox_Buttons/Home.png");
		keyRepair("Images.XBox_Menu", "Resources/Images/Xbox_Buttons/Menu.png");
		keyRepair("Images.XBox_View", "Resources/Images/Xbox_Buttons/View.png");
		keyRepair("Images.XBox_A", "Resources/Images/Xbox_Buttons/A.png");
		keyRepair("Images.XBox_X", "Resources/Images/Xbox_Buttons/X.png");
		keyRepair("Images.XBox_Y", "Resources/Images/Xbox_Buttons/Y.png");
	}
	catch (const boost::property_tree::ptree_bad_data& err)
	{
		Log::Error(std::string{"config value is not convertible to a string: "} + err.what());
	}
	catch (const boost::property_tree::ptree_error& err)
	{
		Log::Error(std::string{"config tree: "} + err.what());
	}
	catch (const std::bad_alloc& err)
	{
		Log::Error(std::string{"out of memory while filling the config: "} + err.what());
	}
}

void ProjectConfig::SaveIni(const std::filesystem::path& filePath) const
{
	try
	{
		boost::property_tree::ini_parser::write_ini(filePath.string(), _pTreeIni);
	}
	catch (const boost::property_tree::ini_parser_error& err)
	{
		Log::Error(std::string{err.what()} + ", cannot save the config - check write permissions and free space");
	}
	//NOTE: the destructor saves too, and anything escaping it is std::terminate
	catch (const std::exception& err)
	{
		Log::Error(std::string{"cannot save the config: "} + err.what());
	}
}
