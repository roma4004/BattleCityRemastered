#include "../headers/SDLEnvironment.h"

class Config;
class Game;

int main(int argc, char* argv[])
{
	constexpr auto width{800};
	constexpr auto height{600};

	auto sdlEnv = SDLEnvironment(width, height);
	const std::unique_ptr<Config> sdl = sdlEnv.Init();
	const std::unique_ptr<Game> game = sdl->CreateGame();

	game->MainLoop();

	return 0;
}
