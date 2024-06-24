#include "../headers/SDLEnvironment.h"

class IConfig;
class IGame;

int main(int argc, char* argv[])
{
	constexpr UPoint windowSize{800, 600};
	auto sdlEnv = SDLEnvironment(windowSize, R"(Resources\Fonts\bc7x7.ttf)");
	const std::unique_ptr<IConfig> sdl = sdlEnv.Init();
	const std::unique_ptr<IGame> game = sdl->CreateGame();

	game->MainLoop();

	return 0;
}
