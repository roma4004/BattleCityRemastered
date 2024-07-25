#include "../Client/GameSuccess.h"
#include "../headers/BulletPool.h"
#include "../headers/Map.h"
#include "../headers/Menu.h"
#include "../headers/pawns/Bullet.h"

#include <algorithm>
#include <iostream>
#include <iostream>
#include <memory>
#include <boost/asio.hpp>
#include <boost/archive/text_oarchive.hpp>

struct Server;

GameSuccess::GameSuccess(const UPoint windowSize, int* windowBuffer, SDL_Renderer* renderer, SDL_Texture* screen,
                         TTF_Font* fpsFont, const std::shared_ptr<EventSystem>& events,
                         std::unique_ptr<InputProviderForMenu>& menuInput,
                         const std::shared_ptr<GameStatistics>& statistics)
	: _windowSize{windowSize},
	  _selectedGameMode{Demo},
	  _currentMode{Demo},
	  _statistics{statistics},
	  _menu{renderer, fpsFont, statistics, windowSize, windowBuffer, menuInput, events},
	  _windowBuffer{windowBuffer},
	  _renderer{renderer},
	  _screen{screen},
	  _fpsFont{fpsFont},
	  _events{events},
	  _bulletPool{std::make_shared<BulletPool>(events, &_allObjects, windowSize, windowBuffer, &_currentMode)},
	  _bonusSystem{events, &_allObjects, windowBuffer, windowSize}
{
	_tankSpawner = std::make_shared<TankSpawner>(windowSize, windowBuffer, &_allObjects, events, _bulletPool);

	ResetBattlefield();
	NextGameMode();
	Subscribe();
}

GameSuccess::~GameSuccess()
{
	Unsubscribe();
}

void GameSuccess::Subscribe()
{
	if (_events == nullptr)
	{
		return;
	}

	_events->AddListener("PreviousGameMode", _name, [this]() { this->PrevGameMode(); });
	_events->AddListener("NextGameMode", _name, [this]() { this->NextGameMode(); });
	_events->AddListener("ResetBattlefield", _name, [this]() { this->ResetBattlefield(); });
	_events->AddListener<const bool>("Pause_Status", _name, [this](const bool newPauseStatus)
	{
		this->_isPause = newPauseStatus;
	});
}

void GameSuccess::Unsubscribe() const
{
	if (_events == nullptr)
	{
		return;
	}

	_events->RemoveListener("PreviousGameMode", _name);
	_events->RemoveListener("NextGameMode", _name);
	_events->RemoveListener("ResetBattlefield", _name);
	_events->RemoveListener<const bool>("Pause_Status", _name);
}

void GameSuccess::ResetBattlefield()
{
	_bulletPool->Clear();
	_currentMode = _selectedGameMode;
	_events->EmitEvent<const GameMode>("GameModeChangedTo", _currentMode);

	_allObjects.clear();
	_allObjects.reserve(1000);

	_statistics->Reset();
	_events->EmitEvent("InitialSpawn");

	//Map creation
	//Map::ObstacleCreation<Brick>(&env, 30,30);
	//Map::ObstacleCreation<Iron>(&env, 310,310);
	const float gridOffset = static_cast<float>(_windowSize.y) / 50.f;
	const Map field{};
	field.MapCreation(&_allObjects, gridOffset, _windowBuffer, _windowSize, _events);
}

void GameSuccess::PrevGameMode()
{
	int mode = _selectedGameMode;
	--mode;

	constexpr int maxMode = static_cast<int>(EndIterator) - 1;
	constexpr int minMode = 1;
	const int newMode = mode < minMode ? maxMode : mode;
	_selectedGameMode = static_cast<GameMode>(newMode);
	_events->EmitEvent<const GameMode>("SelectedGameModeChangedTo", _selectedGameMode);
}

void GameSuccess::NextGameMode()
{
	int mode = _selectedGameMode;
	++mode;

	constexpr int maxMode = static_cast<int>(EndIterator) - 1;
	constexpr int minMode = 1;
	const int newMode = mode > maxMode ? minMode : mode;
	_selectedGameMode = static_cast<GameMode>(newMode);
	_events->EmitEvent<const GameMode>("SelectedGameModeChangedTo", _selectedGameMode);
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
		_mouseButtons.MouseLeftButton = true;
		std::cout << "MouseLeftButton: "
				<< "Down" << '\n';

		return;
	}

	if (event.type == SDL_MOUSEBUTTONUP && event.button.button == SDL_BUTTON_LEFT)
	{
		_mouseButtons.MouseLeftButton = false;
		std::cout << "MouseLeftButton: "
				<< "Up" << '\n';

		return;
	}

	if (event.type == SDL_MOUSEMOTION && _mouseButtons.MouseLeftButton)
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
		case SDLK_p:
			_events->EmitEvent("Pause_Pressed");
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
		case SDLK_p:
			_events->EmitEvent("Pause_Released");
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

void GameSuccess::EventHandling()
{
	// event handling
	SDL_Event event;
	while (SDL_PollEvent(&event))
	{
		if (event.type == SDL_QUIT || (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE))
		{
			_isGameOver = true;
		}

		MouseEvents(event);
		KeyboardEvents(event);
	}
}

void GameSuccess::DisposeDeadObject()
{
	// TODO: separate bullets or somehow delete and recycle in one iterating through _allObjects (because now its two)
	// TODO: create wrapper for bullet and RAII to dispose it to bullet pool
	// Destroy all "dead" objects except bullet they will be recycled
	for (auto it = _allObjects.begin(); it < _allObjects.end();)
	{
		if ((*it)->GetIsAlive() == false)
		{
			if (const auto* bullet = dynamic_cast<Bullet*>(it->get());
				bullet != nullptr)
			{
				_bulletPool->ReturnBullet(*it);
				it = _allObjects.erase(it);
				continue;
			}
		}
		++it;
	}

	const auto it = std::ranges::remove_if(_allObjects, [&](const auto& obj) { return !obj->GetIsAlive(); }).
			begin();
	_allObjects.erase(it, _allObjects.end());
}


void GameSuccess::MainLoop()
{
	try
	{
		boost::asio::io_service io_service;
		boost::asio::ip::tcp::socket socket(io_service);
		// socket.connect(boost::asio::ip::tcp::endpoint(boost::asio::ip::address::from_string("127.0.0.1"), 1234));
		Client client(io_service, "127.0.0.1", "1234", _events);
		std::thread io_service_thread([&io_service]() { io_service.run(); });
		io_service_thread.detach();

		Uint32 frameCount{0};
		Uint32 fps{0};
		float deltaTime{0.f};
		Uint64 oldTime = SDL_GetTicks64();
		auto fpsPrevUpdateTime = oldTime;
		const SDL_Rect fpsRectangle{
				/*x*/ static_cast<int>(_windowSize.x) - 80, /*y*/ 20, /*w*/ 40, /*h*/ 40};

		while (!_isGameOver)
		{
			// Cap to 60 FPS
			// SDL_Delay(static_cast<Uint32>(std::floor(16.666f - deltaTime)));

			ClearBuffer();

			EventHandling();

			_menu.Update();

			if (!_isPause && _currentMode != PlayAsClient)
			{
				_events->EmitEvent<const float>("TickUpdate", deltaTime);
			}

			DisposeDeadObject();

			_events->EmitEvent("RespawnTanks");

			_events->EmitEvent("Draw");

			_events->EmitEvent("DrawHealthBar");
			// TODO: create and blend separate buff layers(objects, effect, interface)

			_events->EmitEvent("DrawMenuBackground");

			const Uint64 newTime = SDL_GetTicks64();
			deltaTime = static_cast<float>(newTime - oldTime) / 1000.0f;
			HandleFPS(frameCount, fpsPrevUpdateTime, fps, newTime);

			// update screen with buffer
			SDL_UpdateTexture(_screen, nullptr, _windowBuffer, static_cast<int>(_windowSize.x) << 2);
			SDL_RenderCopy(_renderer, _screen, nullptr, nullptr);

			_events->EmitEvent("DrawMenuText");

			// Copy the texture with FPS to the renderer
			SDL_RenderCopy(_renderer, _fpsTexture, nullptr, &fpsRectangle);

			SDL_RenderPresent(_renderer);

			oldTime = newTime;
		}

		if (_fpsTexture)
		{
			SDL_DestroyTexture(_fpsTexture);
		}

		// if (io_service_thread.joinable()) {
		// io_service_thread.join();
		// }
	}
	catch (std::exception& e)
	{
		std::cerr << e.what() << '\n';
	}
}

int GameSuccess::Result() const { return 0; }
