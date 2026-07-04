#include "Point.h"
#include "application/SDLEnvironment.h"
#include "enums/GameMode.h"
#include "interfaces/IConfig.h"
#include "interfaces/IGame.h"
#include "utils/NetworkLogger.h"
#include <../headers/application/GameConfig.h>

//TODO: how to improve event system, duplicated code, std::string_view, NRVO, remove std::function, cleanup
int main([[maybe_unused]] int argc, [[maybe_unused]] char* argv[])
{
	NetworkLogger::SetConsoleLogging(true);
	NetworkLogger::SetFileLogging(true, "network_log.txt");
	NetworkLogger::SetVerbosityLevel(1);

	auto gameMode{GameMode::Demo};
	
	GameConfig gameConfig{"x64\\Debug\\config.ini"};
	
	UPoint windowSize{
		.x = gameConfig.pTreeIni.get<unsigned>("Window.width"),
		.y = gameConfig.pTreeIni.get<unsigned>("Window.height")};
	
	UPoint windowPos{
		.x = gameConfig.pTreeIni.get<unsigned>("Window.posX"),
		.y = gameConfig.pTreeIni.get<unsigned>("Window.posY")};
	
	if (argc == 2)
    {
		if (const std::string arg{argv[1]}; arg.ends_with("host"))
		{
			gameMode = GameMode::PlayAsHost;
		}
		else if (arg.ends_with("client"))
		{
			gameMode = GameMode::PlayAsClient;
			windowPos.x = windowPos.x + windowSize.x + 20;
		}
    }
	
	auto sdlEnv = SDLEnvironment(//TODO: refactor to std::filesystem::path and ResourceManager 
			windowSize,
			windowPos,
			gameConfig,
			"Resources/Images/menuSelectorP1.png",
			"Resources/Images/XBoxCon.png",
			"Resources/Images/PS5Con.png"
			);
	const std::unique_ptr<IConfig> sdl = sdlEnv.Init();
	const std::unique_ptr<IGame> game = sdl->CreateGame(gameMode);

	game->MainLoop();

	return 0;
}
