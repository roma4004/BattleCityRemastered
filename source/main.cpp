#include "Point.h"
#include "application/SDLEnvironment.h"
#include "enums/GameMode.h"
#include "interfaces/IConfig.h"
#include "interfaces/IGame.h"
#include "utils/NetworkLogger.h"
#include <../headers/application/GameConfig.h>

//TODO: how to improve event system, duplicated code, std::string_view, NRVO, remove std::function, cleanup
int main(const int argc, char* argv[])
{
	NetworkLogger::SetConsoleLogging(true);
	NetworkLogger::SetFileLogging(true, "network_log.txt");
	NetworkLogger::SetVerbosityLevel(1);

	auto gameMode{GameMode::Demo};

	GameConfig gameConfig{"x64\\Debug\\config.ini"};//TODO: refactor to std::filesystem::path and ResourceManager 

	UPoint windowSize{
			.x = gameConfig.pTreeIni.get<unsigned>("Window.width", 800u),
			.y = gameConfig.pTreeIni.get<unsigned>("Window.height", 600u)};

	UPoint windowPos{
			.x = gameConfig.pTreeIni.get<unsigned>("Window.posX", 100u),
			.y = gameConfig.pTreeIni.get<unsigned>("Window.posY", 100u)};

	UPoint windowsPosOffset{};

	if (argc == 2)
	{
		if (const std::string arg{argv[1]}; arg.ends_with("host"))
		{
			gameMode = GameMode::PlayAsHost;
			windowsPosOffset.x -= windowSize.x / 2;//TODO: input argument windowPos and windowSize
		}
		else if (arg.ends_with("client"))
		{
			gameMode = GameMode::PlayAsClient;
			windowsPosOffset.x += windowSize.x / 2;
		}
	}

	auto sdlEnv = SDLEnvironment(windowSize, windowPos, windowsPosOffset, gameConfig.pTreeIni);
	const std::unique_ptr<IConfig> sdl = sdlEnv.Init();
	const std::unique_ptr<IGame> game = sdl->CreateGame(gameMode);

	game->MainLoop();

	return 0;
}
