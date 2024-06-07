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
	const Rectangle playerOneRect{gridSize * 16.f, static_cast<float>(_windowSize.y) - tankSize, tankSize, tankSize};
	const Rectangle playerTwoRect{gridSize * 32.f, static_cast<float>(_windowSize.y) - tankSize, tankSize, tankSize};

	allPawns.clear();
	allPawns.reserve(1000);

	auto name = "game";
	constexpr int yellow = 0xeaea00;
	if (currentMode == OnePlayer || currentMode == TwoPlayers || currentMode == CoopAI)
	{
		_events->RemoveListener("ArrowUp_Released", name);
		_events->RemoveListener("ArrowUp_Pressed", name);
		_events->RemoveListener("ArrowDown_Released", name);
		_events->RemoveListener("ArrowDown_Pressed", name);
		_events->RemoveListener("Enter_Released", name);
		allPawns.emplace_back(std::make_shared<PlayerOne>(playerOneRect, yellow, tankSpeed, tankHealth, _windowBuffer,
		                                                  _windowSize, &allPawns, _events));
	}

	constexpr int green = 0x408000;
	if (currentMode == TwoPlayers)
	{
		_events->RemoveListener("ArrowUp_Released", name);
		_events->RemoveListener("ArrowUp_Pressed", name);
		_events->RemoveListener("ArrowDown_Released", name);
		_events->RemoveListener("ArrowDown_Pressed", name);
		_events->RemoveListener("Enter_Released", name);
		allPawns.emplace_back(std::make_shared<PlayerTwo>(playerTwoRect, green, tankSpeed, tankHealth, _windowBuffer,
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

		allPawns.emplace_back(std::make_shared<Enemy>(playerOneRect, yellow, tankSpeed, tankHealth, _windowBuffer,
		                                              _windowSize, &allPawns, _events, "PlayerOne", "PlayerTeam"));
		allPawns.emplace_back(std::make_shared<Enemy>(playerTwoRect, green, tankSpeed, tankHealth, _windowBuffer,
		                                              _windowSize, &allPawns, _events, "PlayerTwo", "PlayerTeam"));
	}

	if (currentMode == CoopAI)
	{
		allPawns.emplace_back(std::make_shared<Enemy>(playerTwoRect, green, tankSpeed, tankHealth, _windowBuffer,
		                                              _windowSize, &allPawns, _events, "PlayerTwo", "PlayerTeam"));
	}

	const Rectangle enemyOneRect{gridSize * 16.f - tankSize * 2.f, 0, tankSize, tankSize};
	constexpr int gray = 0x808080;
	allPawns.emplace_back(std::make_shared<Enemy>(enemyOneRect, gray, tankSpeed, tankHealth, _windowBuffer,
	                                              _windowSize, &allPawns, _events, "EnemyOne", "EnemyTeam"));

	const Rectangle enemyTwoRect{gridSize * 32.f - tankSize * 2.f, 0, tankSize, tankSize};
	allPawns.emplace_back(std::make_shared<Enemy>(enemyTwoRect, gray, tankSpeed, tankHealth, _windowBuffer,
	                                              _windowSize, &allPawns, _events, "EnemyTwo", "EnemyTeam"));

	const Rectangle enemyThreeRect{gridSize * 16.f + tankSize * 2.f, 0, tankSize, tankSize};
	allPawns.emplace_back(std::make_shared<Enemy>(enemyThreeRect, gray, tankSpeed, tankHealth, _windowBuffer,
	                                              _windowSize, &allPawns, _events, "EnemyThree", "EnemyTeam"));

	const Rectangle enemyFourRect{gridSize * 32.f + tankSize * 2.f, 0, tankSize, tankSize};
	allPawns.emplace_back(std::make_shared<Enemy>(enemyFourRect, gray, tankSpeed, tankHealth, _windowBuffer,
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
	int mode = currentMode;
	--mode;

	constexpr int maxMode = static_cast<int>(EndIterator) - 1;
	constexpr int minMode = 1;
	const int newMode = mode < minMode ? maxMode : mode;
	currentMode = static_cast<GameMode>(newMode);
}

void GameSuccess::NextGameMode()
{
	int mode = currentMode;
	++mode;

	constexpr int maxMode = static_cast<int>(EndIterator) - 1;
	constexpr int minMode = 1;
	const int newMode = mode > maxMode ? minMode : mode;
	currentMode = static_cast<GameMode>(newMode);
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

void GameSuccess::textToRender(SDL_Renderer* renderer, Point pos, const SDL_Color color, const std::string& text) const
{
	SDL_Surface* surface = TTF_RenderText_Solid(_fpsFont, text.c_str(), color);
	SDL_Texture* texture = SDL_CreateTextureFromSurface(_renderer, surface);

	const SDL_Rect textRect{pos.x, pos.y, surface->w, surface->h};
	SDL_RenderCopy(renderer, texture, nullptr, &textRect);

	SDL_FreeSurface(surface);
	SDL_DestroyTexture(texture);
}

void GameSuccess::HandleMenuText(SDL_Renderer* renderer, const UPoint menuBackgroundPos)
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
	Point pos = {static_cast<int>(menuBackgroundPos.x - 350), static_cast<int>(menuBackgroundPos.y - 350)};
	SDL_Color сolor = {0xff, 0xff, 0xff, 0xff};

	textToRender(renderer, {pos.x - 70, pos.y - 100}, сolor, "BATTLE CITY REMASTERED");

	textToRender(renderer, pos, сolor, currentMode == OnePlayer ? ">ONE PLAYER" : "ONE PLAYER");

	textToRender(renderer, {pos.x, pos.y + 50}, сolor, currentMode == TwoPlayers ? ">TWO PLAYER" : "TWO PLAYER");

	textToRender(renderer, {pos.x, pos.y + 100}, сolor, currentMode == CoopAI ? ">COOP AI" : "COOP AI");
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
			HandleMenuText(_renderer, menu._pos);
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
