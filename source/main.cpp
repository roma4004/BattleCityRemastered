#include "application/GameConfig.h"
#include "application/SDL_Config.h"
#include "enums/GameMode.h"
#include "interfaces/IConfig.h"
#include "interfaces/IGame.h"
#include "utils/NetworkLogger.h"

//TODO: how to improve event system, duplicated code, std::string_view, NRVO, remove std::function, cleanup
int main(const int argc, char* argv[])
{
	NetworkLogger::SetConsoleLogging(true);
	NetworkLogger::SetFileLogging(true, "network_log.txt");
	NetworkLogger::SetVerbosityLevel(1);

	auto gameMode{GameMode::Demo};

	GameConfig gameConfig{"config.ini"};//TODO: refactor to std::filesystem::path and ResourceManager 

	if (argc == 2)
	{
		//TODO: input argument windowPos and windowSize as parameter to gameConfig
		if (const std::string arg{argv[1]}; arg.ends_with("host"))
		{
			gameMode = GameMode::PlayAsHost;
			gameConfig.ApplyWindowOffsetAsHost();
		}
		else if (arg.ends_with("client"))
		{
			gameMode = GameMode::PlayAsClient;
			gameConfig.ApplyWindowOffsetAsClient();
			//TODO: add feature mute intro music when start as client, to play only from host
		}
	}

	SDL_Config sdlEnv{gameConfig};
	const std::unique_ptr<IConfig> sdl = sdlEnv.Init();
	const std::unique_ptr<IGame> game = sdl->CreateGame(gameMode, sdlEnv);

	game->MainLoop();

	return 0;
}
