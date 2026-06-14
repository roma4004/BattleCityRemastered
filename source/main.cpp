#include "Point.h"
#include "application/SDLEnvironment.h"
#include "interfaces/IConfig.h"
#include "interfaces/IGame.h"
#include "utils/NetworkLogger.h"

//TODO: how to improve event system, duplicated code, std::string_view, NRVO, remove std::function, cleanup
int main([[maybe_unused]] int argc, [[maybe_unused]] char* argv[])
{
	NetworkLogger::SetConsoleLogging(true);
	NetworkLogger::SetFileLogging(true, "network_log.txt");
	NetworkLogger::SetVerbosityLevel(1);

	constexpr UPoint windowSize{.x = 800, .y = 600};
	auto sdlEnv = SDLEnvironment(
			windowSize,
			"Resources/Fonts/PressStart2P-vaV7.ttf",//TODO: refactor to std::filesystem::path and ResourceManager 
			"Resources/Images/Title.png",
			"Resources/Sounds/levelStarted.wav",
			"Resources/Images/SpriteSheet.png",
			"Resources/Images/menuSelectorP1.png",
			"Resources/Images/XBoxCon.png",
			"Resources/Images/PS5Con.png"
			);
	const std::unique_ptr<IConfig> sdl = sdlEnv.Init();
	const std::unique_ptr<IGame> game = sdl->CreateGame();

	game->MainLoop();

	return 0;
}
