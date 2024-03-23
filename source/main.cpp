#include "../headers/Environment.h"
#include "../headers/Map.h"
#include "../headers/PlayerOne.h"
#include "../headers/PlayerTwo.h"
#include <iostream>

static void MouseEvents(Environment& env, const SDL_Event& event)
{
	if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT)
	{
		env.mouseButtons.MouseLeftButton = true;
		std::cout << "MouseLeftButton: "
			<< "Down" << '\n';

		return;
	}
	else if (event.type == SDL_MOUSEBUTTONUP && event.button.button == SDL_BUTTON_LEFT)
	{
		env.mouseButtons.MouseLeftButton = false;
		std::cout << "MouseLeftButton: "
			<< "Up" << '\n';

		return;
	}

	if (event.type == SDL_MOUSEMOTION && env.mouseButtons.MouseLeftButton)
	{
		const Sint32 x = event.motion.x;
		const Sint32 y = event.motion.y;
		std::cout << "x: " << x << " \t y: " << y << '\n';
		// const int rowSize = env.windowWidth; ???

		if (x < 1 || y < 1 || x >= env.windowWidth - 1 && y >= env.windowHeight - 1)
		{
			return;
		}
	}
}

void ClearBuffer(const Environment& env)
{
	for (int y = 0; y < env.windowHeight; ++y)
	{
		for (int x = 0; x < env.windowWidth; ++x)
		{
			env.SetPixel(x, y, 0x0);
		}
	}
}

int Init(Environment& env)
{
	if (SDL_Init(SDL_INIT_EVERYTHING) != 0)
	{
		std::cerr << "SDL_Init Error: " << SDL_GetError() << '\n';
		return 1;
	}

	env.window = SDL_CreateWindow("Battle City remastered", 100, 100, env.windowWidth, env.windowHeight, SDL_WINDOW_SHOWN);
	if (env.window == nullptr)
	{
		std::cerr << "SDL_CreateWindow Error: " << SDL_GetError() << '\n';
		return 1;
	}

	env.renderer = SDL_CreateRenderer(env.window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
	if (env.renderer == nullptr)
	{
		std::cerr << "SDL_CreateRenderer Error: " << SDL_GetError() << '\n';
		return 1;
	}

	env.screen = SDL_CreateTexture(env.renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET, env.windowWidth,
								   env.windowHeight);
	if (env.screen == nullptr)
	{
		std::cerr << "Screen SDL_CreateTexture Error: " << SDL_GetError() << '\n';
		return 1;
	}

	const int bufferSize = env.windowWidth * env.windowHeight;
	env.windowBuffer = new int[bufferSize];

	return 0;
}

int main(int argc, char* argv[])
{
	Environment env;
	Init(env);
	Point playerOnePos{env.gridSize * 16, env.windowHeight - env.tankSize};
	Point playerTwoPos{env.gridSize * 32, env.windowHeight - env.tankSize};
	env.allPawns.reserve(2);
	env.allPawns.emplace_back(std::make_unique<PlayerOne>(playerOnePos, env.tankSize, env.tankSize, 0xeaea00, env.tankSpeed,
														  env.tankHealth, &env));
	env.allPawns.emplace_back(std::make_unique<PlayerTwo>(playerTwoPos, env.tankSize, env.tankSize, 0x408000, env.tankSpeed,
														  env.tankHealth, &env));

	//Map creation
	//Map::ObstacleCreation<Brick>(&env, 30,30);
	//Map::ObstacleCreation<Iron>(&env, 310,310);
	Map field{};
	field.MapCreation(&env);

	Uint64 oldTime = SDL_GetTicks64();
	while (!env.isGameOver)
	{
		const Uint64 newTime = SDL_GetTicks64();
		env.deltaTime = static_cast<float>(newTime - oldTime) / 1000.0f;
		const float fps = 1.0f / env.deltaTime;

		std::cout << "fps: " << fps << '\n'; // TODO:use sdl2 ttf here
		std::cout << "deltaTime: " << env.deltaTime << '\n'; // TODO:use sdl2 ttf here

		// Cap to 60 FPS
		// SDL_Delay(floor(16.666f - env.deltaTime));

		ClearBuffer(env);

		// event handling
		while (SDL_PollEvent(&env.event))
		{
			if (env.event.type == SDL_QUIT)
			{
				env.isGameOver = true;
			}

			MouseEvents(env, env.event);

			// TODO: refactor events to handle pawns , objects and other obstacles
			for (auto& object : env.allPawns)
			{
				if (auto* pawn = dynamic_cast<Pawn*>(object.get()))
				{
					pawn->KeyboardEvensHandlers(env, env.event.type, env.event.key.keysym.sym);
				}
			}
		}

		env.events.EmitEvent("TickUpdate");

		// TODO: solve not work because iterator invalidates after call delete this and unsubscribe
		// Destroy all "dead" objects (excluding mapBlocks)
		auto it = std::ranges::remove_if(env.allPawns, [&](const auto& obj)
		{
			return !obj->GetIsAlive();
		}).begin();
		env.allPawns.erase(it, env.allPawns.end());

		env.events.EmitEvent("Draw");

		// update screen with buffer
		SDL_UpdateTexture(env.screen, nullptr, env.windowBuffer, env.windowWidth << 2);
		SDL_RenderCopy(env.renderer, env.screen, nullptr, nullptr);

		SDL_RenderPresent(env.renderer);

		oldTime = newTime;
	}

	SDL_DestroyRenderer(env.renderer);
	SDL_DestroyWindow(env.window);

	delete env.windowBuffer;

	SDL_Quit();

	return 0;
}
