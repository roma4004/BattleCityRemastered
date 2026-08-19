#include "application/ProjectConfig.h"
#include "utils/Log.h"
#include <utility>
#include <boost/property_tree/ini_parser.hpp>

ProjectConfig::ProjectConfig(std::filesystem::path filePath, const bool skipIni)
	: _filePath(std::move(filePath))
	, _skipIniLoad{skipIni}
{
	if (_skipIniLoad)
	{
		DefaultInitIni();
		return;
	}

	if (const auto loaded = LoadIni(_filePath); !loaded)
	{
		//NOTE: nothing to clear - read_ini builds into a local tree and swaps it in only on success
		DefaultInitIni();

		//NOTE: no file yet is just a first run - write the defaults. A file that opened and failed
		//to parse is the user's: overwriting it destroys the line they need to find, so it is left
		//alone here and by the destructor.
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

ProjectConfig::~ProjectConfig()
{
	//NOTE: _loadError means the file is there and unparseable - saving would overwrite it with
	//the defaults, which the constructor already refused to do
	if (!_skipIniLoad && !_loadError)
	{
		SaveIni(_filePath);
	}
}

std::filesystem::path ProjectConfig::ResourcePath(const std::string& key) const
{
	return Get<std::string>(key, key + " path from config.ini");
}

std::expected<void, ConfigError> ProjectConfig::LoadIni(const std::filesystem::path& filePath)
{
	try
	{
		boost::property_tree::ini_parser::read_ini(filePath.string(), _pTreeIni);
	}
	catch (const boost::property_tree::ini_parser_error& err)
	{
		//NOTE: boost reports line 0 for a file it could not open and a 1-based line for bad
		//contents - the two need opposite handling upstream
		return std::unexpected(ConfigError{.path = filePath, .reason = err.message(), .line = err.line()});
	}

	return {};
}

void ProjectConfig::DefaultInitIni()
{
	try
	{
		Set("Window.width", 800u);
		Set("Window.height", 600u);
		Set("Window.posX", 100u);
		Set("Window.posY", 100u);
		Set("Window.vsync", false);
		Set("Window.MonitorNumber", 1u);

		Set("Sound.volume", 100u);
		Set("Sound.onOff", true);

		Set("Control.swap", false);

		Set("Fonts.BattleCity", "Resources/Fonts/PressStart2P-vaV7.ttf");
		Set("Images.Logo", "Resources/Images/Title.png");
		Set("Music.LevelStarted", "Resources/Sounds/levelStarted.wav");
		Set("Images.SpriteSheet", "Resources/Images/SpriteSheet.png");
		Set("Images.MenuSelectorP1", "Resources/Images/menuSelectorP1.png");

		Set("Images.PS5_Create", "Resources/Images/PS5_Buttons/Create.png");
		Set("Images.PS5_Cross", "Resources/Images/PS5_Buttons/Cross.png");
		Set("Images.PS5_D-Pad", "Resources/Images/PS5_Buttons/D-Pad.png");
		Set("Images.PS5_Home", "Resources/Images/PS5_Buttons/Home.png");
		Set("Images.PS5_Options", "Resources/Images/PS5_Buttons/Options.png");
		Set("Images.PS5_Triangle", "Resources/Images/PS5_Buttons/Triangle.png");

		Set("Images.XBox_D-Pad", "Resources/Images/XBox_Buttons/D-Pad.png");
		Set("Images.XBox_Home", "Resources/Images/XBox_Buttons/Home.png");
		Set("Images.XBox_Menu", "Resources/Images/XBox_Buttons/Menu.png");
		Set("Images.XBox_View", "Resources/Images/XBox_Buttons/View.png");
		Set("Images.XBox_A", "Resources/Images/XBox_Buttons/A.png");
		Set("Images.XBox_Y", "Resources/Images/XBox_Buttons/Y.png");
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

//TODO: add feature save window last position
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
}
