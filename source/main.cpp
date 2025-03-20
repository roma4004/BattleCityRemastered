#include "../headers/Point.h"
#include "../headers/application/SDLEnvironment.h"
#include "../headers/interfaces/IConfig.h"
#include "../headers/interfaces/IGame.h"

int main([[maybe_unused]] int argc, [[maybe_unused]] char* argv[])
{
	constexpr UPoint windowSize{.x = 800, .y = 600};
	auto sdlEnv = SDLEnvironment(
			windowSize,
			"Resources/Fonts/PressStart2P-vaV7.ttf",
			"Resources/Images/Title.png",
			"Resources/Sounds/levelStarted.wav");
	const std::unique_ptr<IConfig> sdl = sdlEnv.Init();
	const std::unique_ptr<IGame> game = sdl->CreateGame();

	game->MainLoop();

	return 0;
}
