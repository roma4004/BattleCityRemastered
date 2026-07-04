#include "../../headers/application/GameConfig.h"
#include <iostream>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>

GameConfig::GameConfig(const std::string& filePath)
{
	LoadIni(filePath);
}

GameConfig::~GameConfig() = default;

void GameConfig::LoadIni(const std::string& filePath)
{
	try
	{
		boost::property_tree::ini_parser::read_ini(filePath, pTreeIni);
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
		pTreeIni.put("Window.width", 800u);
		pTreeIni.put("Window.height", 600u);
		pTreeIni.put("Window.posX", 100u);
		pTreeIni.put("Window.posY", 100u);
		pTreeIni.put("Window.vsync", false);

		pTreeIni.put("Sound.volume", 100u);
		pTreeIni.put("Sound.onOff", true);

		pTreeIni.put("Control.swap", false);

		pTreeIni.put("Fonts.BattleCity", "Resources/Fonts/PressStart2P-vaV7.ttf");
		pTreeIni.put("Images.Logo", "Resources/Images/Title.png");
		pTreeIni.put("Music.LevelStarted", "Resources/Sounds/levelStarted.wav");
		pTreeIni.put("Images.SpriteSheet", "Resources/Images/SpriteSheet.png");
		pTreeIni.put("Images.MenuSelectorP1", "Resources/Images/menuSelectorP1.png");

		pTreeIni.put("Images.PS5_Create", "Resources/Images/PS5_Buttons/Create.png");
		pTreeIni.put("Images.PS5_Cross", "Resources/Images/PS5_Buttons/Cross.png");
		pTreeIni.put("Images.PS5_D-Pad", "Resources/Images/PS5_Buttons/D-Pad.png");
		pTreeIni.put("Images.PS5_Home", "Resources/Images/PS5_Buttons/Home.png");
		pTreeIni.put("Images.PS5_Options", "Resources/Images/PS5_Buttons/Options.png");
		pTreeIni.put("Images.PS5_Triangle", "Resources/Images/PS5_Buttons/Triangle.png");

		pTreeIni.put("Images.XBox_D-Pad", "Resources/Images/XBox_Buttons/D-Pad.png");
		pTreeIni.put("Images.XBox_Home", "Resources/Images/XBox_Buttons/Home.png");
		pTreeIni.put("Images.XBox_Menu", "Resources/Images/XBox_Buttons/Menu.png");
		pTreeIni.put("Images.XBox_View", "Resources/Images/XBox_Buttons/View.png");
		pTreeIni.put("Images.XBox_A", "Resources/Images/XBox_Buttons/A.png");
		pTreeIni.put("Images.XBox_Y", "Resources/Images/XBox_Buttons/Y.png");
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

void GameConfig::SaveIni(const std::string& filePath) const
{
	try
	{
		boost::property_tree::ini_parser::write_ini(filePath, pTreeIni);
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
