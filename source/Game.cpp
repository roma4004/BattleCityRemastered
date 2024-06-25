#include "../headers/Game.h"
#include "../headers/Enemy.h"
#include "../headers/Map.h"
#include "../headers/PlayerOne.h"
#include "../headers/PlayerTwo.h"

#include <algorithm>
#include <cmath>
#include <iostream>

GameSuccess::GameSuccess(const UPoint windowSize, int* windowBuffer, SDL_Renderer* renderer, SDL_Texture* screen,
                         TTF_Font* fpsFont)
	: _windowSize{windowSize}, _windowBuffer{windowBuffer}, _renderer{renderer}, _screen{screen}, _fpsFont{fpsFont},
	  _events{std::make_shared<EventSystem>()}
{
	const float gridSize = static_cast<float>(_windowSize.y) / 50.f;
	constexpr float tankSpeed = 142;
	constexpr int tankHealth = 100;
	const float tankSize = gridSize * 3;// for better turns
	Rectangle playerOneRect{gridSize * 16.f, static_cast<float>(_windowSize.y) - tankSize, tankSize, tankSize};
	Rectangle playerTwoRect{gridSize * 32.f, static_cast<float>(_windowSize.y) - tankSize, tankSize, tankSize};
	Rectangle enemyOneRect{gridSize * 16.f - tankSize * 2.f, 0, tankSize, tankSize};
	Rectangle enemyTwoRect{gridSize * 32.f - tankSize * 2.f, 0, tankSize, tankSize};
	Rectangle enemyThreeRect{gridSize * 16.f + tankSize * 2.f, 0, tankSize, tankSize};
	Rectangle enemyFourRect{gridSize * 32.f + tankSize * 2.f, 0, tankSize, tankSize};
	allPawns.reserve(6);
	allPawns.emplace_back(std::make_shared<PlayerOne>(playerOneRect, 0xeaea00, tankSpeed, tankHealth, _windowBuffer,
	                                                  _windowSize, &allPawns, _events));
	allPawns.emplace_back(std::make_shared<PlayerTwo>(playerTwoRect, 0x408000, tankSpeed, tankHealth, _windowBuffer,
	                                                  _windowSize, &allPawns, _events));
	allPawns.emplace_back(std::make_shared<Enemy>(enemyOneRect, 0x808080, tankSpeed, tankHealth, _windowBuffer,
	                                              _windowSize, &allPawns, _events, "EnemyOne"));
	allPawns.emplace_back(std::make_shared<Enemy>(enemyTwoRect, 0x808080, tankSpeed, tankHealth, _windowBuffer,
	                                              _windowSize, &allPawns, _events, "EnemyTwo"));
	allPawns.emplace_back(std::make_shared<Enemy>(enemyThreeRect, 0x808080, tankSpeed, tankHealth, _windowBuffer,
	                                              _windowSize, &allPawns, _events, "EnemyThree"));
	allPawns.emplace_back(std::make_shared<Enemy>(enemyFourRect, 0x808080, tankSpeed, tankHealth, _windowBuffer,
	                                              _windowSize, &allPawns, _events, "EnemyFour"));

	//Map creation
	//Map::ObstacleCreation<Brick>(&env, 30,30);
	//Map::ObstacleCreation<Iron>(&env, 310,310);
	const Map field{};
	field.MapCreation(&allPawns, gridSize, _windowBuffer, _windowSize, _events);
}

void GameSuccess::ClearBuffer() const
{
	const auto size = _windowSize.x * _windowSize.y * sizeof(int);
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

		if (x < 1 || y < 1 ||
		    x >= static_cast<Sint32>(_windowSize.x) - 1 && y >= static_cast<Sint32>(_windowSize.y) - 1)
		{
			return;
		}
	}
}

void GameSuccess::KeyPressed(const SDL_Event& event) const
{
	switch (event.key.keysym.sym)
	{
		case SDLK_w:
			_events->EmitEvent("W_Pressed");
			break;
		case SDLK_a:
			_events->EmitEvent("A_Pressed");
			break;
		case SDLK_s:
			_events->EmitEvent("S_Pressed");
			break;
		case SDLK_d:
			_events->EmitEvent("D_Pressed");
			break;
		case SDLK_SPACE:
			_events->EmitEvent("Space_Pressed");
			break;
		case SDLK_UP:
			_events->EmitEvent("ArrowUp_Pressed");
			break;
		case SDLK_LEFT:
			_events->EmitEvent("ArrowLeft_Pressed");
			break;
		case SDLK_DOWN:
			_events->EmitEvent("ArrowDown_Pressed");
			break;
		case SDLK_RIGHT:
			_events->EmitEvent("ArrowRight_Pressed");
			break;
		case SDLK_RCTRL:
			_events->EmitEvent("RCTRL_Pressed");
			break;
		default:
			break;
	}
}

void GameSuccess::KeyReleased(const SDL_Event& event) const
{
	switch (event.key.keysym.sym)
	{
		case SDLK_w:
			_events->EmitEvent("W_Released");
			break;
		case SDLK_a:
			_events->EmitEvent("A_Released");
			break;
		case SDLK_s:
			_events->EmitEvent("S_Released");
			break;
		case SDLK_d:
			_events->EmitEvent("D_Released");
			break;
		case SDLK_SPACE:
			_events->EmitEvent("Space_Released");
			break;
		case SDLK_UP:
			_events->EmitEvent("ArrowUp_Released");
			break;
		case SDLK_LEFT:
			_events->EmitEvent("ArrowLeft_Released");
			break;
		case SDLK_DOWN:
			_events->EmitEvent("ArrowDown_Released");
			break;
		case SDLK_RIGHT:
			_events->EmitEvent("ArrowRight_Released");
			break;
		case SDLK_RCTRL:
			_events->EmitEvent("RCTRL_Released");
			break;
		default:
			break;
	}
}

void GameSuccess::KeyboardEvents(const SDL_Event& event) const
{
	if (event.type == SDL_KEYDOWN)
	{
		KeyPressed(event);
	}
	else if (event.type == SDL_KEYUP)
	{
		KeyReleased(event);
	}
}

void GameSuccess::MainLoop()
{
	constexpr SDL_Color fpsColor{140, 0, 255, 0};
	Uint32 frameCount{0};
	Uint32 fps{0};
	bool isGameOver{false};
	float deltaTime{0.f};
	SDL_Event event{};
	Uint64 oldTime = SDL_GetTicks64();
	auto fpsPrevUpdateTime = oldTime;
	const SDL_Rect fpsRectangle{
			/*x*/ static_cast<int>(_windowSize.x) - 80, /*y*/ 20, /*w*/ 40, /*h*/ 40};

	while (!isGameOver)
	{
		// Cap to 60 FPS
		SDL_Delay(static_cast<Uint32>(std::floor(16.666f - deltaTime)));

		ClearBuffer();

		// event handling
		while (SDL_PollEvent(&event))
		{
			if (event.type == SDL_QUIT || (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE))
			{
				isGameOver = true;
			}

			MouseEvents(event);
			KeyboardEvents(event);
		}

		_events->EmitEvent<float>("TickUpdate", deltaTime);

		// TODO: solve not work because iterator invalidates after call delete this and unsubscribe
		// Destroy all "dead" objects (excluding mapBlocks)
		const auto it = std::ranges::remove_if(allPawns, [&](const auto& obj) { return !obj->GetIsAlive(); }).begin();
		allPawns.erase(it, allPawns.end());

		_events->EmitEvent("Draw");

		const Uint64 newTime = SDL_GetTicks64();
		deltaTime = static_cast<float>(newTime - oldTime) / 1000.0f;
		++frameCount;
		if (newTime - fpsPrevUpdateTime >= 1000)
		{
			if (fps != frameCount)
			{
				fps = frameCount;

				_fpsSurface = TTF_RenderText_Solid(_fpsFont, std::to_string(fps).c_str(), fpsColor);
				if (_fpsTexture)
				{
					SDL_DestroyTexture(_fpsTexture);
				}
				_fpsTexture = SDL_CreateTextureFromSurface(_renderer, _fpsSurface);

				SDL_FreeSurface(_fpsSurface);
			}
			fpsPrevUpdateTime = newTime;
			frameCount = 0;
		}

		// update screen with buffer
		SDL_UpdateTexture(_screen, nullptr, _windowBuffer, static_cast<int>(_windowSize.x) << 2);
		SDL_RenderCopy(_renderer, _screen, nullptr, nullptr);

		// Copy the texture with FPS to the renderer
		SDL_RenderCopy(_renderer, _fpsTexture, nullptr, &fpsRectangle);

		SDL_RenderPresent(_renderer);

		oldTime = newTime;
	}

	if (_fpsTexture)
	{
		SDL_DestroyTexture(_fpsTexture);
	}
}
