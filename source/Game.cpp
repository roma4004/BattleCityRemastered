#include "../headers/Game.h"

GameSuccess::GameSuccess(const size_t windowWidth, const size_t windowHeight, int* windowBuffer, SDL_Renderer* renderer,
						 SDL_Texture* screen)
	: _windowWidth(windowWidth), _windowHeight(windowHeight), _windowBuffer(windowBuffer), _renderer(renderer),
	  _screen(screen)
{
	_events = std::make_shared<EventSystem>();
	const float gridSize = _windowHeight / 50.f;
	constexpr float tankSpeed = 142;
	constexpr int tankHealth = 100;
	const float tankSize = gridSize * 3;// for better turns
	FPoint playerOnePos{gridSize * 16.f, static_cast<float>(_windowHeight) - tankSize};
	FPoint playerTwoPos{gridSize * 32.f, static_cast<float>(_windowHeight) - tankSize};
	allPawns.reserve(2);
	allPawns.emplace_back(std::make_shared<PlayerOne>(playerOnePos, tankSize, tankSize, 0xeaea00, tankSpeed, tankHealth,
													  _windowBuffer, _windowWidth, _windowHeight, &allPawns, _events));
	allPawns.emplace_back(std::make_shared<PlayerTwo>(playerTwoPos, tankSize, tankSize, 0x408000, tankSpeed, tankHealth,
													  _windowBuffer, _windowWidth, _windowHeight, &allPawns, _events));

	//Map creation
	//Map::ObstacleCreation<Brick>(&env, 30,30);
	//Map::ObstacleCreation<Iron>(&env, 310,310);
	const Map field{};
	field.MapCreation(&allPawns, gridSize, _windowBuffer, _windowWidth, _windowHeight, _events);
}

void GameSuccess::ClearBuffer() const
{
	const auto size = _windowWidth * _windowHeight * sizeof(int);
	memset(_windowBuffer, 0, size);
}

void GameSuccess::MouseEvents(const SDL_Event& event)
{
	if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT)
	{
		mouseButtons.MouseLeftButton = true;
		std::cout << "MouseLeftButton: "
				  << "Down" << '\n';

		return;
	}
	if (event.type == SDL_MOUSEBUTTONUP && event.button.button == SDL_BUTTON_LEFT)
	{
		mouseButtons.MouseLeftButton = false;
		std::cout << "MouseLeftButton: "
				  << "Up" << '\n';

		return;
	}

	if (event.type == SDL_MOUSEMOTION && mouseButtons.MouseLeftButton)
	{
		const Sint32 x = event.motion.x;
		const Sint32 y = event.motion.y;
		std::cout << "x: " << x << " \t y: " << y << '\n';
		// const int rowSize = env.windowWidth; ???

		if (x < 1 || y < 1 || x >= static_cast<Sint32>(_windowWidth) - 1 && y >= static_cast<Sint32>(_windowHeight) - 1)
		{
			return;
		}
	}
}

void GameSuccess::MainLoop()
{
	bool isGameOver{false};
	float deltaTime = 0.f;
	SDL_Event event{};
	Uint64 oldTime = SDL_GetTicks64();
	while (!isGameOver)
	{
		const Uint64 newTime = SDL_GetTicks64();
		deltaTime = static_cast<float>(newTime - oldTime) / 1000.0f;
		const float fps = 1.0f / deltaTime;

		std::cout << "fps: " << fps << '\n';			// TODO:use sdl2 ttf here
		std::cout << "deltaTime: " << deltaTime << '\n';// TODO:use sdl2 ttf here

		// Cap to 60 FPS
		// SDL_Delay(floor(16.666f - env.deltaTime));

		ClearBuffer();

		// event handling
		while (SDL_PollEvent(&event))
		{
			if (event.type == SDL_QUIT)
			{
				isGameOver = true;
			}

			MouseEvents(event);

			// TODO: refactor events to handle pawns, objects and other obstacles
			for (auto& object: allPawns)
			{
				if (auto* pawn = dynamic_cast<Pawn*>(object.get()))
				{
					pawn->KeyboardEvensHandlers(event.type, event.key.keysym.sym);
				}
			}
		}

		_events->EmitEvent<float>("TickUpdate", deltaTime);

		// TODO: solve not work because iterator invalidates after call delete this and unsubscribe
		// Destroy all "dead" objects (excluding mapBlocks)
		const auto it = std::ranges::remove_if(allPawns, [&](const auto& obj) { return !obj->GetIsAlive(); }).begin();
		allPawns.erase(it, allPawns.end());

		_events->EmitEvent("Draw");

		// update screen with buffer
		SDL_UpdateTexture(_screen, nullptr, _windowBuffer, static_cast<int>(_windowWidth) << 2);
		SDL_RenderCopy(_renderer, _screen, nullptr, nullptr);

		SDL_RenderPresent(_renderer);

		oldTime = newTime;
	}
}
