#include "application/CommandLineParser.h"
#include "application/GameConfig.h"
#include "application/SDL_Config.h"
#include "interfaces/IConfig.h"
#include "interfaces/IGame.h"
#include "utils/NetworkLogger.h"

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
	const std::unique_ptr<IConfig> sdl = sdlEnv.Init();
	const std::unique_ptr<IGame> game = sdl->CreateGame(launchOptions.gameMode, sdlEnv);

	game->MainLoop();

	return 0;
}
