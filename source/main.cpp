#include "../headers/SDLEnvironment.h"

class Config;
class Game;

int main(int argc, char* argv[])
{
	constexpr UPoint windowSize{800, 600};

	auto sdlEnv = SDLEnvironment(windowSize);
	const std::unique_ptr<Config> sdl = sdlEnv.Init();
	const std::unique_ptr<Game> game = sdl->CreateGame();

	game->MainLoop();

	return 0;
}
