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
	_events->AddListener("Menu_Released", name, [this]() { ToggleMenu(); });
	_events->AddListener("Statistics_Enemy1_Died", name, [this]() { statistics.enemyOneNeedRespawn = true; });
	_events->AddListener("Statistics_Enemy2_Died", name, [this]() { statistics.enemyTwoNeedRespawn = true; });
	_events->AddListener("Statistics_Enemy3_Died", name, [this]() { statistics.enemyThreeNeedRespawn = true; });
	_events->AddListener("Statistics_Enemy4_Died", name, [this]() { statistics.enemyFourNeedRespawn = true; });
	_events->AddListener("Statistics_P1_Died", name, [this]() { statistics.playerOneNeedRespawn = true; });
	_events->AddListener("Statistics_P2_Died", name, [this]() { statistics.playerTwoNeedRespawn = true; });
	_events->AddListener("Statistics_CoopOneAI_Died", name, [this]() { statistics.coopOneAINeedRespawn = true; });
	_events->AddListener("Statistics_CoopTwoAI_Died", name, [this]() { statistics.coopTwoAINeedRespawn = true; });

	ResetBattlefield();
}

GameSuccess::~GameSuccess()
{
	_events->RemoveListener("Menu_Released", name);
	_events->RemoveListener("Statistics_Enemy1_Died", name);
	_events->RemoveListener("Statistics_Enemy2_Died", name);
	_events->RemoveListener("Statistics_Enemy3_Died", name);
	_events->RemoveListener("Statistics_Enemy4_Died", name);
	_events->RemoveListener("Statistics_P1_Died", name);
	_events->RemoveListener("Statistics_P2_Died", name);
	_events->RemoveListener("Statistics_CoopOneAI_Died", name);
	_events->RemoveListener("Statistics_CoopTwoAI_Died", name);
}

void GameSuccess::SpawnEnemyTanks(const float gridOffset, const float speed, const int health, const float size)
{
	SpawnEnemy(statistics.enemyOneNeedRespawn, 1, gridOffset, speed, health, size);
	SpawnEnemy(statistics.enemyTwoNeedRespawn, 2, gridOffset, speed, health, size);
	SpawnEnemy(statistics.enemyThreeNeedRespawn, 3, gridOffset, speed, health, size);
	SpawnEnemy(statistics.enemyFourNeedRespawn, 4, gridOffset, speed, health, size);
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

void GameSuccess::ResetBattlefield()
{
	allPawns.clear();
	allPawns.reserve(1000);

	statistics.enemyRespawnResource = 20;
	statistics.playerOneRespawnResource = 3;
	statistics.playerTwoRespawnResource = 3;

	statistics.enemyOneNeedRespawn = false;
	statistics.enemyTwoNeedRespawn = false;
	statistics.enemyThreeNeedRespawn = false;
	statistics.enemyFourNeedRespawn = false;

	statistics.playerOneNeedRespawn = false;
	statistics.playerTwoNeedRespawn = false;
	statistics.coopOneAINeedRespawn = false;
	statistics.coopTwoAINeedRespawn = false;

	const float gridSize = static_cast<float>(_windowSize.y) / 50.f;
	constexpr float tankSpeed = 142;
	constexpr int tankHealth = 100;
	const float tankSize = gridSize * 3;

	SpawnPlayerTanks(gridSize, tankSpeed, tankHealth, tankSize);

	SpawnEnemyTanks(gridSize, tankSpeed, tankHealth, tankSize);

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

void GameSuccess::TextToRender(SDL_Renderer* renderer, const Point pos, const SDL_Color color,
                               const std::string& text) const
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
	const Point pos = {static_cast<int>(menuBackgroundPos.x - 350), static_cast<int>(menuBackgroundPos.y - 350)};
	constexpr SDL_Color сolor = {0xff, 0xff, 0xff, 0xff};

	TextToRender(renderer, {pos.x - 70, pos.y - 100}, сolor, "BATTLE CITY REMASTERED");

	TextToRender(renderer, pos, сolor, currentMode == OnePlayer ? ">ONE PLAYER" : "ONE PLAYER");

	TextToRender(renderer, {pos.x, pos.y + 50}, сolor, currentMode == TwoPlayers ? ">TWO PLAYER" : "TWO PLAYER");

	TextToRender(renderer, {pos.x, pos.y + 100}, сolor, currentMode == CoopWithAI ? ">COOP AI" : "COOP AI");

	constexpr SDL_Color сolorStat = {0x00, 0xff, 0xff, 0xff};
	TextToRender(renderer, {pos.x - 100, pos.y + 150}, сolorStat, "GAME STATISTICS");
	TextToRender(renderer, {pos.x - 100, pos.y + 200}, сolorStat,
	             "ENEMY RESPAWN REMAIN " + std::to_string(statistics.enemyRespawnResource));
	TextToRender(renderer, {pos.x - 100, pos.y + 250}, сolorStat,
	             "P1 RESPAWN REMAIN " + std::to_string(statistics.playerOneRespawnResource));
	TextToRender(renderer, {pos.x - 100, pos.y + 300}, сolorStat,
	             "P2 RESPAWN REMAIN " + std::to_string(statistics.playerTwoRespawnResource));

}

void GameSuccess::HandleFPS(Uint32& frameCount, Uint64& fpsPrevUpdateTime, Uint32& fps, const Uint64 newTime)
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

void GameSuccess::SpawnPlayerTanks(const float gridOffset, const float speed, const int health, const float size)
{
	ToggleMenu();
	if (currentMode == OnePlayer || currentMode == TwoPlayers || currentMode == CoopWithAI)
	{
		SpawnP1(gridOffset, speed, health, size);
	}

	if (currentMode == TwoPlayers)
	{
		SpawnP2(gridOffset, speed, health, size);
	}

	if (currentMode == Demo)
	{
		SpawnCoop1(gridOffset, speed, health, size);
		SpawnCoop2(gridOffset, speed, health, size);
	}

	if (currentMode == CoopWithAI)
	{
		SpawnCoop2(gridOffset, speed, health, size);
	}
}

void GameSuccess::SpawnEnemy(bool& needSpawn, const int enemyIndex, const float gridOffset, const float speed,
                             const int health,
                             const float size)
{
	std::vector<Rectangle> spawnPos{
			{gridOffset * 16.f - size * 2.f, 0, size, size},
			{gridOffset * 32.f - size * 2.f, 0, size, size},
			{gridOffset * 16.f + size * 2.f, 0, size, size},
			{gridOffset * 32.f + size * 2.f, 0, size, size}
	};
	for (auto& spawnSpot: spawnPos)
	{
		bool isFreeSpawnSpot = true;
		for (const std::shared_ptr<BaseObj>& pawn: allPawns)
		{
			if (pawn && IsCollideWith(spawnSpot, pawn->GetShape()))
			{
				isFreeSpawnSpot = false;
			}
		}

		if (isFreeSpawnSpot)
		{
			constexpr int gray = 0x808080;
			allPawns.emplace_back(std::make_shared<Enemy>(spawnSpot, gray, speed, health, _windowBuffer, _windowSize,
			                                              &allPawns, _events, "Enemy" + std::to_string(enemyIndex),
			                                              "EnemyTeam"));
			--statistics.enemyRespawnResource;
			needSpawn = false;
			return;
		}
	}
}

void GameSuccess::SpawnP1(const float gridOffset, const float speed, const int health, const float size)
{
	const float windowSizeY = static_cast<float>(_windowSize.y);
	const Rectangle playerOneRect{gridOffset * 16.f, windowSizeY - size, size, size};
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
		allPawns.emplace_back(std::make_shared<PlayerOne>(playerOneRect, yellow, speed, health, _windowBuffer,
		                                                  _windowSize, &allPawns, _events));
		statistics.playerOneNeedRespawn = false;
		--statistics.playerOneRespawnResource;
	}
}

void GameSuccess::SpawnP2(const float gridOffset, const float speed, const int health, const float size)
{
	const float windowSizeY = static_cast<float>(_windowSize.y);
	const Rectangle playerTwoRect{gridOffset * 32.f, windowSizeY - size, size, size};
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
		allPawns.emplace_back(std::make_shared<PlayerTwo>(playerTwoRect, green, speed, health, _windowBuffer,
		                                                  _windowSize, &allPawns, _events));
		statistics.playerTwoNeedRespawn = false;
		--statistics.playerTwoRespawnResource;
	}
}

void GameSuccess::SpawnCoop1(const float gridOffset, const float speed, const int health, const float size)
{
	const float windowSizeY = static_cast<float>(_windowSize.y);
	const Rectangle playerOneRect{gridOffset * 16.f, windowSizeY - size, size, size};
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
		allPawns.emplace_back(std::make_shared<CoopAI>(playerOneRect, yellow, speed, health, _windowBuffer, _windowSize,
		                                               &allPawns, _events, "CoopOneAI", "PlayerTeam"));
		statistics.coopOneAINeedRespawn = false;
		--statistics.playerOneRespawnResource;
	}
}

void GameSuccess::SpawnCoop2(const float gridOffset, const float speed, const int health, const float size)
{
	const float windowSizeY = static_cast<float>(_windowSize.y);
	const Rectangle playerTwoRect{gridOffset * 32.f, windowSizeY - size, size, size};
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
		allPawns.emplace_back(std::make_shared<CoopAI>(playerTwoRect, green, speed, health, _windowBuffer, _windowSize,
		                                               &allPawns, _events, "CoopTwoAI", "PlayerTeam"));
		statistics.coopTwoAINeedRespawn = false;
		--statistics.playerTwoRespawnResource;
	}
}

void GameSuccess::RespawnTanks()
{
	const float gridOffset = static_cast<float>(_windowSize.y) / 50.f;
	const float size = gridOffset * 3;
	constexpr float speed = 142;
	constexpr int health = 100;
	if (statistics.enemyOneNeedRespawn && statistics.enemyRespawnResource > 0)
	{
		SpawnEnemy(statistics.enemyOneNeedRespawn, 1, gridOffset, speed, health, size);
	}
	if (statistics.enemyTwoNeedRespawn && statistics.enemyRespawnResource > 0)
	{
		SpawnEnemy(statistics.enemyTwoNeedRespawn, 2, gridOffset, speed, health, size);
	}
	if (statistics.enemyThreeNeedRespawn && statistics.enemyRespawnResource > 0)
	{
		SpawnEnemy(statistics.enemyThreeNeedRespawn, 3, gridOffset, speed, health, size);
	}
	if (statistics.enemyFourNeedRespawn && statistics.enemyRespawnResource > 0)
	{
		SpawnEnemy(statistics.enemyFourNeedRespawn, 4, gridOffset, speed, health, size);
	}
	if (statistics.playerOneNeedRespawn && statistics.playerOneRespawnResource > 0)
	{
		SpawnP1(gridOffset, speed, health, size);
	}
	if (statistics.playerTwoNeedRespawn && statistics.playerTwoRespawnResource > 0)
	{
		SpawnP2(gridOffset, speed, health, size);
	}
	if (statistics.coopOneAINeedRespawn && statistics.playerOneRespawnResource > 0)
	{
		SpawnCoop1(gridOffset, speed, health, size);
	}
	if (statistics.coopTwoAINeedRespawn && statistics.playerTwoRespawnResource > 0)
	{
		SpawnCoop2(gridOffset, speed, health, size);
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

		RespawnTanks();

		_events->EmitEvent("Draw");

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
