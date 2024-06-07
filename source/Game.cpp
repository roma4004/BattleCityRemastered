#include "../headers/Game.h"
#include "../headers/Enemy.h"
#include "../headers/Map.h"
#include "../headers/Menu.h"
#include "../headers/PlayerOne.h"
#include "../headers/PlayerTwo.h"

#include <algorithm>
#include <cmath>
#include <iostream>

void GameSuccess::ResetBattlefield()
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
	allPawns.clear();
	allPawns.reserve(1000);

	auto name = "game";
	if (currentMode == OnePlayer || currentMode == TwoPlayers || currentMode == CoopAI)
	{
		_events->RemoveListener("ArrowUp_Released", name);
		_events->RemoveListener("ArrowUp_Pressed", name);
		_events->RemoveListener("ArrowDown_Released", name);
		_events->RemoveListener("ArrowDown_Pressed", name);
		_events->RemoveListener("Enter_Released", name);
		allPawns.emplace_back(std::make_shared<PlayerOne>(playerOneRect, 0xeaea00, tankSpeed, tankHealth, _windowBuffer,
		                                                  _windowSize, &allPawns, _events));
	}

	if (currentMode == TwoPlayers)
	{
		_events->RemoveListener("ArrowUp_Released", name);
		_events->RemoveListener("ArrowUp_Pressed", name);
		_events->RemoveListener("ArrowDown_Released", name);
		_events->RemoveListener("ArrowDown_Pressed", name);
		_events->RemoveListener("Enter_Released", name);
		allPawns.emplace_back(std::make_shared<PlayerTwo>(playerTwoRect, 0x408000, tankSpeed, tankHealth, _windowBuffer,
		                                                  _windowSize, &allPawns, _events));
	}

	if (currentMode == Demo)
	{
		_events->AddListener("ArrowUp_Released", name, [this]() { up = true; });
		_events->AddListener("ArrowUp_Pressed", name, [this]() { up = false; });
		_events->AddListener("ArrowDown_Released", name, [this]() { down = true; });
		_events->AddListener("ArrowDown_Pressed", name, [this]() { down = false; });
		_events->AddListener("Enter_Released", name, [this]() { reset = true; });
		_events->AddListener("Menu_Released", name, [this, name]()
		{
			menuShow = !menuShow;
			_events->AddListener("ArrowUp_Released", name, [this]() { up = true; });
			_events->AddListener("ArrowUp_Pressed", name, [this]() { up = false; });
			_events->AddListener("ArrowDown_Released", name, [this]() { down = true; });
			_events->AddListener("ArrowDown_Pressed", name, [this]() { down = false; });
			_events->AddListener("Enter_Released", name, [this]() { reset = true; });
		});

		allPawns.emplace_back(std::make_shared<Enemy>(playerOneRect, 0xeaea00, tankSpeed, tankHealth, _windowBuffer,
		                                              _windowSize, &allPawns, _events, "PlayerOne", "PlayerTeam"));
		allPawns.emplace_back(std::make_shared<Enemy>(playerTwoRect, 0x408000, tankSpeed, tankHealth, _windowBuffer,
		                                              _windowSize, &allPawns, _events, "PlayerTwo", "PlayerTeam"));
	}

	if (currentMode == CoopAI)
	{
		allPawns.emplace_back(std::make_shared<Enemy>(playerTwoRect, 0x408000, tankSpeed, tankHealth, _windowBuffer,
		                                              _windowSize, &allPawns, _events, "PlayerTwo", "PlayerTeam"));
	}

	allPawns.emplace_back(std::make_shared<Enemy>(enemyOneRect, 0x808080, tankSpeed, tankHealth, _windowBuffer,
	                                              _windowSize, &allPawns, _events, "EnemyOne", "EnemyTeam"));
	allPawns.emplace_back(std::make_shared<Enemy>(enemyTwoRect, 0x808080, tankSpeed, tankHealth, _windowBuffer,
	                                              _windowSize, &allPawns, _events, "EnemyTwo", "EnemyTeam"));
	allPawns.emplace_back(std::make_shared<Enemy>(enemyThreeRect, 0x808080, tankSpeed, tankHealth, _windowBuffer,
	                                              _windowSize, &allPawns, _events, "EnemyThree", "EnemyTeam"));
	allPawns.emplace_back(std::make_shared<Enemy>(enemyFourRect, 0x808080, tankSpeed, tankHealth, _windowBuffer,
	                                              _windowSize, &allPawns, _events, "EnemyFour", "EnemyTeam"));

	//Map creation
	//Map::ObstacleCreation<Brick>(&env, 30,30);
	//Map::ObstacleCreation<Iron>(&env, 310,310);
	const Map field{};
	field.MapCreation(&allPawns, gridSize, _windowBuffer, _windowSize, _events);
}

void GameSuccess::SetGameMode(GameMode gameMode) { currentMode = gameMode; }

void GameSuccess::PrevGameMode()
{
	if (currentMode == Demo) { currentMode = CoopAI; }
	else if (currentMode == OnePlayer) { currentMode = CoopAI; }
	else if (currentMode == TwoPlayers) { currentMode = OnePlayer; }
	else
		if (currentMode == CoopAI) { currentMode = TwoPlayers; }
}

void GameSuccess::NextGameMode()
{
	if (currentMode == Demo) { currentMode = OnePlayer; }
	else if (currentMode == OnePlayer) { currentMode = TwoPlayers; }
	else if (currentMode == TwoPlayers) { currentMode = CoopAI; }
	else
		if (currentMode == CoopAI) { currentMode = OnePlayer; }
}

GameSuccess::GameSuccess(const UPoint windowSize, int* windowBuffer, SDL_Renderer* renderer, SDL_Texture* screen,
                         TTF_Font* fpsFont)
	: _windowSize{windowSize}, _windowBuffer{windowBuffer}, _renderer{renderer}, _screen{screen}, _fpsFont{fpsFont},
	  _events{std::make_shared<EventSystem>()}
{
	ResetBattlefield();
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
		case SDLK_RETURN:
			_events->EmitEvent("Enter_Pressed");
			break;
		case SDLK_m:
			_events->EmitEvent("Menu_Pressed");
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
		case SDLK_RETURN:
			_events->EmitEvent("Enter_Released");
			break;
		case SDLK_m:
			_events->EmitEvent("Menu_Released");
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

	Menu menu{_windowSize, _windowBuffer};
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

		if (menuShow)
		{
			menu.BlendToWindowBuffer();
		}

		// update screen with buffer
		SDL_UpdateTexture(_screen, nullptr, _windowBuffer, static_cast<int>(_windowSize.x) << 2);
		SDL_RenderCopy(_renderer, _screen, nullptr, nullptr);

		if (menuShow)
		{
			if (up)
			{
				PrevGameMode();
				up = false;
			}
			else if (down)
			{
				NextGameMode();
				down = false;
			}
			else if (reset)
			{
				ResetBattlefield();
				reset = false;
				menuShow = false;
			}

			//menu text
			Point menuPoint = {static_cast<int>(menu._pos.x - 350), static_cast<int>(menu._pos.y - 350)};
			SDL_Color menuColor = {0xff, 0xff, 0xff, 0xff};

			std::string rowZero = "BATTLE CITY REMASTERED";
			SDL_Surface* rowZeroMenuSurface = TTF_RenderText_Solid(_fpsFont, rowZero.c_str(), menuColor);
			SDL_Texture* rowZeroMenuTexture = SDL_CreateTextureFromSurface(_renderer, rowZeroMenuSurface);
			const SDL_Rect rowZeroTextRect{menuPoint.x - 70, menuPoint.y - 100, rowZeroMenuSurface->w,
			                               rowZeroMenuSurface->h};
			SDL_RenderCopy(_renderer, rowZeroMenuTexture, nullptr, &rowZeroTextRect);
			SDL_FreeSurface(rowZeroMenuSurface);
			SDL_DestroyTexture(rowZeroMenuTexture);

			std::string rowOne = currentMode == OnePlayer ? ">ONE PLAYER" : "ONE PLAYER";
			SDL_Surface* rowOneMenuSurface = TTF_RenderText_Solid(_fpsFont, rowOne.c_str(), menuColor);
			SDL_Texture* rowOneMenuTexture = SDL_CreateTextureFromSurface(_renderer, rowOneMenuSurface);
			const SDL_Rect rowOneTextRect{menuPoint.x, menuPoint.y, rowOneMenuSurface->w, rowOneMenuSurface->h};
			SDL_RenderCopy(_renderer, rowOneMenuTexture, nullptr, &rowOneTextRect);
			SDL_FreeSurface(rowOneMenuSurface);
			SDL_DestroyTexture(rowOneMenuTexture);

			std::string rowTwo = currentMode == TwoPlayers ? ">TWO PLAYER" : "TWO PLAYER";
			SDL_Surface* rowTwoMenuSurface = TTF_RenderText_Solid(_fpsFont, rowTwo.c_str(), menuColor);
			SDL_Texture* rowTwoMenuTexture = SDL_CreateTextureFromSurface(_renderer, rowTwoMenuSurface);
			const SDL_Rect rowTwoTextRect{menuPoint.x, menuPoint.y + 50, rowTwoMenuSurface->w, rowTwoMenuSurface->h};
			SDL_RenderCopy(_renderer, rowTwoMenuTexture, nullptr, &rowTwoTextRect);
			SDL_FreeSurface(rowTwoMenuSurface);
			SDL_DestroyTexture(rowTwoMenuTexture);

			std::string rowThree = currentMode == CoopAI ? ">COOP AI" : "COOP AI";
			SDL_Surface* rowThreeMenuSurface = TTF_RenderText_Solid(_fpsFont, rowThree.c_str(), menuColor);
			SDL_Texture* rowThreeMenuTexture = SDL_CreateTextureFromSurface(_renderer, rowThreeMenuSurface);
			const SDL_Rect rowThreeTextRect{menuPoint.x, menuPoint.y + 100, rowThreeMenuSurface->w,
			                                rowThreeMenuSurface->h};
			SDL_RenderCopy(_renderer, rowThreeMenuTexture, nullptr, &rowThreeTextRect);
			SDL_FreeSurface(rowThreeMenuSurface);
			SDL_DestroyTexture(rowThreeMenuTexture);
		}

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
