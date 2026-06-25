#include "Point.h"
#include "application/SDLEnvironment.h"
#include "enums/GameMode.h"
#include "interfaces/IConfig.h"
#include "interfaces/IGame.h"
#include "utils/NetworkLogger.h"

//TODO: how to improve event system, duplicated code, std::string_view, NRVO, remove std::function, cleanup
int main([[maybe_unused]] int argc, [[maybe_unused]] char* argv[])
{
	NetworkLogger::SetConsoleLogging(true);
	NetworkLogger::SetFileLogging(true, "network_log.txt");
	NetworkLogger::SetVerbosityLevel(1);

	auto gameMode{GameMode::Demo};
	
	constexpr UPoint windowSize{.x = 800, .y = 600};
	UPoint windowPos{.x = 100, .y = 100};
	
	//TODO make config.ini and parser for it 
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
	
	auto sdlEnv = SDLEnvironment(
			windowSize,
			windowPos,
			"Resources/Fonts/PressStart2P-vaV7.ttf",//TODO: refactor to std::filesystem::path and ResourceManager 
			"Resources/Images/Title.png",
			"Resources/Sounds/levelStarted.wav",
			"Resources/Images/SpriteSheet.png",
			"Resources/Images/menuSelectorP1.png",
			"Resources/Images/XBoxCon.png",
			"Resources/Images/PS5Con.png"
			);
	const std::unique_ptr<IConfig> sdl = sdlEnv.Init();
	const std::unique_ptr<IGame> game = sdl->CreateGame(gameMode);

	game->MainLoop();

	return 0;
}
