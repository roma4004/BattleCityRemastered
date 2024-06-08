#include "../headers/Game.h"
#include "../headers/CoopAI.h"
#include "../headers/Enemy.h"
#include "../headers/Map.h"
#include "../headers/Menu.h"
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
	ResetBattlefield();

	_events->AddListener("Menu_Released", name, [this]() { ToggleMenu(); });
	_events->AddListener("Statistics_Enemy_Respawn", name, [this]() { --statistics.enemyResurrectionCount; });
	_events->AddListener("Statistics_P1_Respawn", name, [this]() { --statistics.playerOneResurrectionCount; });
	_events->AddListener("Statistics_P2_Respawn", name, [this]() { --statistics.playerTwoResurrectionCount; });
	_events->AddListener("Statistics_CoopOneAI_Respawn", name, [this]() { --statistics.playerOneResurrectionCount; });
	_events->AddListener("Statistics_CoopTwoAI_Respawn", name, [this]() { --statistics.playerTwoResurrectionCount; });
	_events->AddListener("Statistics_Enemy_Died", name, [this]()
	{
		++statistics.enemyNeedRespawn;
	});
	_events->AddListener("Statistics_P1_Died", name, [this]() { statistics.playerOneNeedRespawn = true; });
	_events->AddListener("Statistics_P2_Died", name, [this]() { statistics.playerTwoNeedRespawn = true; });
	_events->AddListener("Statistics_CoopOneAI_Died", name, [this]() { statistics.coopOneAINeedRespawn = true; });
	_events->AddListener("Statistics_CoopTwoAI_Died", name, [this]() { statistics.coopTwoAINeedRespawn = true; });
	_events->AddListener("Statistics_Reset", name, [this]()
	{
		statistics.enemyResurrectionCount = 20;
		statistics.playerOneResurrectionCount = 3;
		statistics.playerTwoResurrectionCount = 3;
		statistics.enemyNeedRespawn = 0;
		statistics.playerOneNeedRespawn = false;
		statistics.playerTwoNeedRespawn = false;
	});
}

GameSuccess::~GameSuccess()
{
	_events->RemoveListener("Menu_Released", name);
	_events->RemoveListener("Statistics_Enemy_Respawn", name);
	_events->RemoveListener("Statistics_P1_Respawn", name);
	_events->RemoveListener("Statistics_P2_Respawn", name);
	_events->RemoveListener("Statistics_CoopOneAI_Respawn", name);
	_events->RemoveListener("Statistics_CoopTwoAI_Respawn", name);
	_events->RemoveListener("Statistics_Enemy_Died", name);
	_events->RemoveListener("Statistics_P1_Died", name);
	_events->RemoveListener("Statistics_P2_Died", name);
	_events->RemoveListener("Statistics_CoopOneAI_Died", name);
	_events->RemoveListener("Statistics_CoopTwoAI_Died", name);
	_events->RemoveListener("Statistics_Reset", name);
}

void GameSuccess::CreateEnemiesTanks(const float gridSize, const float tankSpeed, const int tankHealth,
                                     const float tankSize)
{
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
}

void GameSuccess::ToggleMenu()
{
	menuKeys.menuShow = !menuKeys.menuShow;
	if (menuKeys.menuShow)
	{
		_events->AddListener("ArrowUp_Released", name, [&btn = menuKeys]() { btn.up = true; });
		_events->AddListener("ArrowUp_Pressed", name, [&btn = menuKeys]() { btn.up = false; });
		_events->AddListener("ArrowDown_Released", name, [&btn = menuKeys]() { btn.down = true; });
		_events->AddListener("ArrowDown_Pressed", name, [&btn = menuKeys]() { btn.down = false; });
		_events->AddListener("Enter_Released", name, [&btn = menuKeys]() { btn.reset = true; });
	}
	else
	{
		_events->RemoveListener("ArrowUp_Released", name);
		_events->RemoveListener("ArrowUp_Pressed", name);
		_events->RemoveListener("ArrowDown_Released", name);
		_events->RemoveListener("ArrowDown_Pressed", name);
		_events->RemoveListener("Enter_Released", name);
	}
}

void GameSuccess::CreatePlayerTanks(const float gridSize, const float tankSpeed, const int tankHealth,
                                    const float tankSize)
{
	const float windowSizeY = static_cast<float>(_windowSize.y);
	const Rectangle playerOneRect{gridSize * 16.f, windowSizeY - tankSize, tankSize, tankSize};
	const Rectangle playerTwoRect{gridSize * 32.f, windowSizeY - tankSize, tankSize, tankSize};

	ToggleMenu();
	constexpr int yellow = 0xeaea00;
	if (currentMode == OnePlayer || currentMode == TwoPlayers || currentMode == CoopWithAI)
	{
		allPawns.emplace_back(std::make_shared<PlayerOne>(playerOneRect, yellow, tankSpeed, tankHealth, _windowBuffer,
		                                                  _windowSize, &allPawns, _events));
	}

	constexpr int green = 0x408000;
	if (currentMode == TwoPlayers)
	{
		allPawns.emplace_back(std::make_shared<PlayerTwo>(playerTwoRect, green, tankSpeed, tankHealth, _windowBuffer,
		                                                  _windowSize, &allPawns, _events));
	}

	if (currentMode == Demo)
	{
		allPawns.emplace_back(std::make_shared<CoopAI>(playerOneRect, yellow, tankSpeed, tankHealth, _windowBuffer,
		                                               _windowSize, &allPawns, _events, "CoopOneAI", "PlayerTeam"));
		allPawns.emplace_back(std::make_shared<CoopAI>(playerTwoRect, green, tankSpeed, tankHealth, _windowBuffer,
		                                               _windowSize, &allPawns, _events, "CoopTwoAI", "PlayerTeam"));
	}

	if (currentMode == CoopWithAI)
	{
		allPawns.emplace_back(std::make_shared<CoopAI>(playerTwoRect, green, tankSpeed, tankHealth, _windowBuffer,
		                                               _windowSize, &allPawns, _events, "CoopTwoAI", "PlayerTeam"));
	}
}

void GameSuccess::ResetBattlefield()
{
	_events->EmitEvent("Statistics_Reset");
	allPawns.clear();
	allPawns.reserve(1000);

	const float gridSize = static_cast<float>(_windowSize.y) / 50.f;
	constexpr float tankSpeed = 142;
	constexpr int tankHealth = 100;
	const float tankSize = gridSize * 3;

	statistics.enemyResurrectionCount = 20;
	statistics.playerOneResurrectionCount = 3;
	statistics.playerTwoResurrectionCount = 3;
	statistics.enemyNeedRespawn = 0;
	statistics.playerOneNeedRespawn = false;
	statistics.playerTwoNeedRespawn = false;

	CreatePlayerTanks(gridSize, tankSpeed, tankHealth, tankSize);

	CreateEnemiesTanks(gridSize, tankSpeed, tankHealth, tankSize);

	//Map creation
	//Map::ObstacleCreation<Brick>(&env, 30,30);
	//Map::ObstacleCreation<Iron>(&env, 310,310);
	const Map field{};
	field.MapCreation(&allPawns, gridSize, _windowBuffer, _windowSize, _events);
}

void GameSuccess::SetGameMode(const GameMode gameMode) { currentMode = gameMode; }

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

void GameSuccess::TextToRender(SDL_Renderer* renderer, Point pos, const SDL_Color color, const std::string& text) const
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
	if (menuKeys.up)
	{
		PrevGameMode();
		menuKeys.up = false;
	}
	else if (menuKeys.down)
	{
		NextGameMode();
		menuKeys.down = false;
	}
	else if (menuKeys.reset)
	{
		ResetBattlefield();
		menuKeys.reset = false;
		menuKeys.menuShow = false;
	}

	//menu text
	Point pos = {static_cast<int>(menuBackgroundPos.x - 350), static_cast<int>(menuBackgroundPos.y - 350)};
	SDL_Color сolor = {0xff, 0xff, 0xff, 0xff};

	TextToRender(renderer, {pos.x - 70, pos.y - 100}, сolor, "BATTLE CITY REMASTERED");

	TextToRender(renderer, pos, сolor, currentMode == OnePlayer ? ">ONE PLAYER" : "ONE PLAYER");

	TextToRender(renderer, {pos.x, pos.y + 50}, сolor, currentMode == TwoPlayers ? ">TWO PLAYER" : "TWO PLAYER");

	TextToRender(renderer, {pos.x, pos.y + 100}, сolor, currentMode == CoopWithAI ? ">COOP AI" : "COOP AI");

	SDL_Color сolorStat = {0x00, 0xff, 0xff, 0xff};
	TextToRender(renderer, {pos.x - 100, pos.y + 150}, сolorStat, "GAME STATISTICS");
	TextToRender(renderer, {pos.x - 100, pos.y + 200}, сolorStat,
	             "ENEMY RESPAWN REMAIN " + std::to_string(statistics.enemyResurrectionCount));
	TextToRender(renderer, {pos.x - 100, pos.y + 250}, сolorStat,
	             "P1 RESPAWN REMAIN " + std::to_string(statistics.playerOneResurrectionCount));
	TextToRender(renderer, {pos.x - 100, pos.y + 300}, сolorStat,
	             "P2 RESPAWN REMAIN " + std::to_string(statistics.playerTwoResurrectionCount));

}

void GameSuccess::HandleFPS(Uint32& frameCount, Uint64& fpsPrevUpdateTime, Uint32 fps, const Uint64 newTime)
{
	++frameCount;
	if (newTime - fpsPrevUpdateTime >= 1000)
	{
		if (fps != frameCount)
		{
			fps = frameCount;

			_fpsSurface = TTF_RenderText_Solid(_fpsFont, std::to_string(fps).c_str(), SDL_Color{140, 0, 255, 0});
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
}

bool GameSuccess::IsCollideWith(const Rectangle& r1, const Rectangle& r2)
{
	// Check if one rectangle is to the right of the other
	if (r1.x > r2.x + r2.w || r2.x > r1.x + r1.w)
	{
		return false;
	}

	// Check if one rectangle is above the other
	if (r1.y > r2.y + r2.h || r2.y > r1.y + r1.h)
	{
		return false;
	}

	// If neither of the above conditions are met, the rectangles overlap
	return true;
}

void GameSuccess::SpawnEnemy(const float gridSize, const float tankSpeed, const int tankHealth, const float tankSize,
                             const int gray)
{
	std::vector<Rectangle> spawnPos{
			{gridSize * 16.f - tankSize * 2.f, 0, tankSize, tankSize},
			{gridSize * 32.f - tankSize * 2.f, 0, tankSize, tankSize},
			{gridSize * 16.f + tankSize * 2.f, 0, tankSize, tankSize},
			{gridSize * 32.f + tankSize * 2.f, 0, tankSize, tankSize}
	};
	for (size_t i = 0; i < spawnPos.size(); ++i)
	{
		bool isFreeSpawnSpot = true;
		for (const std::shared_ptr<BaseObj>& pawn: allPawns)
		{
			if (IsCollideWith(spawnPos[i], pawn->GetShape()))
			{
				isFreeSpawnSpot = false;
			}
		}

		if (isFreeSpawnSpot)
		{
			allPawns.emplace_back(std::make_shared<Enemy>(spawnPos[i], gray, tankSpeed, tankHealth,
			                                              _windowBuffer, _windowSize, &allPawns, _events,
			                                              "Enemy" + std::to_string(i), "EnemyTeam"));
			--statistics.enemyNeedRespawn;
			return;
		}
	}
}

void GameSuccess::SpawnP1(const float gridSize, const float tankSpeed, const int tankHealth, const float tankSize)
{
	const float windowSizeY = static_cast<float>(_windowSize.y);
	const Rectangle playerOneRect{gridSize * 16.f, windowSizeY - tankSize, tankSize, tankSize};
	bool isFreeSpawnSpot = true;
	for (const std::shared_ptr<BaseObj>& pawn: allPawns)
	{
		if (IsCollideWith(playerOneRect, pawn->GetShape()))
		{
			isFreeSpawnSpot = false;
		}
	}

	if (isFreeSpawnSpot)
	{
		constexpr int yellow = 0xeaea00;
		allPawns.emplace_back(std::make_shared<PlayerOne>(playerOneRect, yellow, tankSpeed, tankHealth,
														  _windowBuffer, _windowSize, &allPawns, _events));
		statistics.playerOneNeedRespawn = false;
	}
}

void GameSuccess::SpawnP2(const float gridSize, const float tankSpeed, const int tankHealth, const float tankSize)
{
	const float windowSizeY = static_cast<float>(_windowSize.y);
	const Rectangle playerTwoRect{gridSize * 32.f, windowSizeY - tankSize, tankSize, tankSize};
	bool isFreeSpawnSpot = true;
	for (const std::shared_ptr<BaseObj>& pawn: allPawns)
	{
		if (IsCollideWith(playerTwoRect, pawn->GetShape()))
		{
			isFreeSpawnSpot = false;
		}
	}

	if (isFreeSpawnSpot)
	{
		constexpr int green = 0x408000;
		allPawns.emplace_back(std::make_shared<PlayerTwo>(playerTwoRect, green, tankSpeed, tankHealth,
														  _windowBuffer, _windowSize, &allPawns, _events));
		statistics.playerTwoNeedRespawn = false;
	}
}

void GameSuccess::SpawnCoop1(const float gridSize, const float tankSpeed, const int tankHealth, const float tankSize)
{
	const float windowSizeY = static_cast<float>(_windowSize.y);
	const Rectangle playerOneRect{gridSize * 16.f, windowSizeY - tankSize, tankSize, tankSize};
	bool isFreeSpawnSpot = true;
	for (const std::shared_ptr<BaseObj>& pawn: allPawns)
	{
		if (IsCollideWith(playerOneRect, pawn->GetShape()))
		{
			isFreeSpawnSpot = false;
		}
	}

	if (isFreeSpawnSpot)
	{
		constexpr int yellow = 0xeaea00;
		allPawns.emplace_back(std::make_shared<CoopAI>(playerOneRect, yellow, tankSpeed, tankHealth,
													   _windowBuffer, _windowSize, &allPawns, _events, "CoopOneAI",
													   "PlayerTeam"));
		statistics.coopOneAINeedRespawn = false;
	}
}

void GameSuccess::SpawnCoop2(const float gridSize, const float tankSpeed, const int tankHealth, const float tankSize)
{
	const float windowSizeY = static_cast<float>(_windowSize.y);
	const Rectangle playerTwoRect{gridSize * 32.f, windowSizeY - tankSize, tankSize, tankSize};
	bool isFreeSpawnSpot = true;
	for (const std::shared_ptr<BaseObj>& pawn: allPawns)
	{
		if (IsCollideWith(playerTwoRect, pawn->GetShape()))
		{
			isFreeSpawnSpot = false;
		}
	}

	if (isFreeSpawnSpot)
	{
		constexpr int green = 0x408000;
		allPawns.emplace_back(std::make_shared<CoopAI>(playerTwoRect, green, tankSpeed, tankHealth,
													   _windowBuffer, _windowSize, &allPawns, _events, "CoopTwoAI",
													   "PlayerTeam"));
		statistics.coopTwoAINeedRespawn = false;
	}
}

void GameSuccess::RespawnTanks()
{
	const float gridSize = static_cast<float>(_windowSize.y) / 50.f;
	constexpr float tankSpeed = 142;
	constexpr int tankHealth = 100;
	const float tankSize = gridSize * 3;
	constexpr int gray = 0x808080;
	if (statistics.enemyNeedRespawn > 0 && statistics.enemyResurrectionCount > 0)
	{
		SpawnEnemy(gridSize, tankSpeed, tankHealth, tankSize, gray);
	}
	if (statistics.playerOneNeedRespawn && statistics.playerOneResurrectionCount > 0)
	{
		SpawnP1(gridSize, tankSpeed, tankHealth, tankSize);
	}
	if (statistics.playerTwoNeedRespawn && statistics.playerTwoResurrectionCount > 0)
	{
		SpawnP2(gridSize, tankSpeed, tankHealth, tankSize);
	}
	if (statistics.coopOneAINeedRespawn && statistics.playerOneResurrectionCount > 0)
	{
		SpawnCoop1(gridSize, tankSpeed, tankHealth, tankSize);
	}
	if (statistics.coopTwoAINeedRespawn && statistics.playerTwoResurrectionCount > 0)
	{
		SpawnCoop2(gridSize, tankSpeed, tankHealth, tankSize);
	}
}

void GameSuccess::MainLoop()
{
	Uint32 frameCount{0};
	Uint32 fps{0};
	bool isGameOver{false};
	float deltaTime{0.f};
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
		SDL_Event event;
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

		RespawnTanks();

		const Uint64 newTime = SDL_GetTicks64();
		deltaTime = static_cast<float>(newTime - oldTime) / 1000.0f;
		HandleFPS(frameCount, fpsPrevUpdateTime, fps, newTime);

		if (menuKeys.menuShow)
		{
			menu.BlendToWindowBuffer();
		}

		// update screen with buffer
		SDL_UpdateTexture(_screen, nullptr, _windowBuffer, static_cast<int>(_windowSize.x) << 2);
		SDL_RenderCopy(_renderer, _screen, nullptr, nullptr);

		if (menuKeys.menuShow)
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
