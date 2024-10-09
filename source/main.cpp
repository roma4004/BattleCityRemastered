#include "../headers/SDLEnvironment.h"

class IConfig;
class IGame;

int main([[maybe_unused]] int argc, [[maybe_unused]] char* argv[])
{
	constexpr UPoint windowSize{800, 600};
	auto sdlEnv = SDLEnvironment(windowSize, R"(Resources\Fonts\bc7x7.ttf)",
	                             R"(Resources\Images\Title.png)",
	                             R"(Resources\Sounds\levelStarted.wav)");
	const std::unique_ptr<IConfig> sdl = sdlEnv.Init();
	const std::unique_ptr<IGame> game = sdl->CreateGame();

	game->MainLoop();

	return 0;
}
