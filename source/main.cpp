#include "../headers/Environment.h"
#include "../headers/PlayerOne.h"
#include "../headers/PlayerTwo.h"
#include <iostream>

static void MouseEvents(Environment& env, const SDL_Event& event)
{
	if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
		env.MouseButtons.MouseLeftButton = true;
		std::cout << "MouseLeftButton: "
				  << "Down" << '\n';

		return;
	} else if (event.type == SDL_MOUSEBUTTONUP && event.button.button == SDL_BUTTON_LEFT) {
		env.MouseButtons.MouseLeftButton = false;
		std::cout << "MouseLeftButton: "
				  << "Up" << '\n';

		return;
	}

	if (event.type == SDL_MOUSEMOTION && env.MouseButtons.MouseLeftButton) {
		const Sint32 x = event.motion.x;
		const Sint32 y = event.motion.y;
		std::cout << "x: " << x << " \t y: " << y << '\n';
		// const int rowSize = env.windowWidth; ???

		if (x < 1 || y < 1 || x >= env.WindowWidth - 1 && y >= env.WindowHeight - 1) {
			return;
		}
	}
}

void ClearBuffer(const Environment& env)
{
	for (int y = 0; y < env.WindowHeight; ++y) {
		for (int x = 0; x < env.WindowWidth; ++x) {
			env.SetPixel(x, y, 0x0);
		}
	}
}

int Init(Environment& env)
{
	if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
		std::cerr << "SDL_Init Error: " << SDL_GetError() << '\n';
		return 1;
	}

	env.Window = SDL_CreateWindow("Battle City remastered", 100, 100, env.WindowWidth, env.WindowHeight, SDL_WINDOW_SHOWN);
	if (env.Window == nullptr) {
		std::cerr << "SDL_CreateWindow Error: " << SDL_GetError() << '\n';
		return 1;
	}

	env.Renderer = SDL_CreateRenderer(env.Window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
	if (env.Renderer == nullptr) {
		std::cerr << "SDL_CreateRenderer Error: " << SDL_GetError() << '\n';
		return 1;
	}

	env.Screen = SDL_CreateTexture(env.Renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET, env.WindowWidth,
								   env.WindowHeight);
	if (env.Screen == nullptr) {
		std::cerr << "Screen SDL_CreateTexture Error: " << SDL_GetError() << '\n';
		return 1;
	}

	const int bufferSize = env.WindowWidth * env.WindowHeight;
	env.WindowBuffer = new int[bufferSize];

	return 0;
}

int main(int argc, char* argv[])
{
	Environment env;
	constexpr int speed = 4;
	constexpr int tankHealth = 100;
	constexpr Point playerOnePos{20, 20};
	constexpr Point playerTwoPos{200, 200};
	env.AllPawns.reserve(2);
	env.AllPawns.emplace_back(new PlayerOne{playerOnePos, 100, 100, 0x00ff00, speed, tankHealth, &env});
	env.AllPawns.emplace_back(new PlayerTwo{playerTwoPos, 100, 100, 0xff0000, speed, tankHealth, &env});

	Init(env);
	while (!env.IsGameOver) {
		ClearBuffer(env);

		// event handling
		while (SDL_PollEvent(&env.Event)) {
			if (env.Event.type == SDL_QUIT) {
				env.IsGameOver = true;
			}

			MouseEvents(env, env.Event);

			for (auto* pawn: env.AllPawns) {
				pawn->KeyboardEvensHandlers(env, env.Event.type, env.Event.key.keysym.sym);
			}
		}

		env.Events.EmitEvent("TickUpdate");

		env.Events.EmitEvent("MarkDestroy"); //TODO: solve not work because iterator invalidates after call delete this and unsubscribe
		// Destroy all dead objects
		for (const auto* pawn : env.PawnsToDestroy) {
			delete pawn;
		}
		if (!env.PawnsToDestroy.empty()) {
			env.PawnsToDestroy.clear();
		}

		env.Events.EmitEvent("Draw");

		// update screen with buffer
		SDL_UpdateTexture(env.Screen, nullptr, env.WindowBuffer, env.WindowWidth << 2);
		SDL_RenderCopy(env.Renderer, env.Screen, nullptr, nullptr);

		SDL_RenderPresent(env.Renderer);
	}

	SDL_DestroyRenderer(env.Renderer);
	SDL_DestroyWindow(env.Window);

	delete env.WindowBuffer;

	SDL_Quit();

	return 0;
}
