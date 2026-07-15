#include "../../headers/application/GameConfig.h"
#include <iostream>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>

GameConfig::GameConfig(std::string filePath)
	: _filePath(std::move(filePath))
{
	LoadIni(_filePath);

	windowSize = UPoint{.x = Get<unsigned>("Window.width", 800u),
						.y = Get<unsigned>("Window.height", 600u)};

	windowPos = UPoint{.x = Get<unsigned>("Window.posX", 100u),
					   .y = Get<unsigned>("Window.posY", 100u)};
}

GameConfig::~GameConfig()
{
	SaveIni(_filePath);
}; //TODO: investigate exception, rewrite destructor

void GameConfig::LoadIni(const std::string& filePath)
{
	try
	{
		boost::property_tree::ini_parser::read_ini(filePath, _pTreeIni);
		return;
	}
	catch (const boost::property_tree::ini_parser_error& err)
	{
		std::cout << err.what() << ", will be used default settings" << '\n';
	}

	DefaultInitIni(filePath);
}

void GameConfig::DefaultInitIni(const std::string& filePath)
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
		// Catches data conversion errors inside PropertyTree
		std::cerr << "Data error: Type cannot be converted to a string! " << err.what() << '\n';
	}
	catch (const boost::property_tree::ptree_error& err)
	{
		// Generic catch for any PropertyTree-specific errors
		std::cerr << "PropertyTree error: " << err.what() << '\n';
	}
	catch (const std::bad_alloc& err)
	{
		// Catches critical out-of-memory errors
		std::cerr << "Critical error: Out of memory! " << err.what() << '\n';
	}

	SaveIni(filePath);
}

//TODO: add feature save window last position
void GameConfig::SaveIni(const std::string& filePath) const
{
	try
	{
		boost::property_tree::ini_parser::write_ini(filePath, _pTreeIni);
	}
	catch (const boost::property_tree::ini_parser_error& err)
	{
		std::cerr << err.what() <<
				", cannot save default config.ini. Check your write permissions or free space on your drive" << '\n';
	}
}

// 2. Read specific keys using dot notation (Section.Key)
//std::string dbHost = pt.get<std::string>("Settings.theme");
//int dbPort = pt.get<int>("Database.port");

void GameConfig::ApplyWindowOffsetAsHost() { windowsPosOffset.x -= windowSize.x / 2; }

void GameConfig::ApplyWindowOffsetAsClient() { windowsPosOffset.x += windowSize.x / 2; }
