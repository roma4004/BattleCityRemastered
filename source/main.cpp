#include "application/CommandLineParser.h"
#include "application/Game.h"
#include "application/GameConfig.h"
#include "application/SDL_Config.h"
#include "utils/NetworkLogger.h"
#include <iostream>

//TODO: how to improve event system, duplicated code, std::string_view, NRVO, remove std::function, cleanup
int main(const int argc, char* argv[])
{
	NetworkLogger::SetConsoleLogging(true);
	NetworkLogger::SetFileLogging(true, "network_log.txt");
	NetworkLogger::SetVerbosityLevel(1);

	const LaunchOptions launchOptions = CommandLineParser::Parse(argc, argv);

	GameConfig gameConfig{"config.ini"};//TODO: refactor to std::filesystem::path and ResourceManager
	gameConfig.Apply(launchOptions);

	SDL_Config sdlEnv{gameConfig};
	if (const auto init = sdlEnv.Init(); !init)
	{
		std::cerr << init.error().stage << ": " << init.error().detail << '\n';

		return 1;
	}

	Game game{gameConfig, sdlEnv, launchOptions.gameMode};
	game.Run();

	return game.Result();
}
