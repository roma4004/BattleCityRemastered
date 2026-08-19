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

	const auto launchOptions = CommandLineParser::Parse(argc, argv);
	if (!launchOptions)
	{
		std::cerr << "bad argument '" << launchOptions.error().arg << "': " << launchOptions.error().reason << '\n';

		return 1;
	}

	GameConfig gameConfig{"config.ini"};//TODO: refactor to std::filesystem::path and ResourceManager
	//NOTE: not fatal - defaults play fine. Said out loud because the file is kept as it is, so
	//otherwise the settings would just look ignored.
	if (const auto& configError = gameConfig.LoadError())
	{
		std::cerr << "config " << configError->path << " line " << configError->line << ": " << configError->reason
				  << ", running on defaults and leaving the file untouched" << '\n';
	}

	gameConfig.Apply(*launchOptions);

	SDL_Config sdlEnv{gameConfig};
	if (const auto init = sdlEnv.Init(); !init)
	{
		std::cerr << init.error().stage << ": " << init.error().detail << '\n';

		return 1;
	}

	Game game{gameConfig, sdlEnv, launchOptions->gameMode};
	game.Run();

	return game.Result();
}
