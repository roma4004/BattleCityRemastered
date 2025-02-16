#include "../headers/GameSuccess.h"
#include "../headers/BulletPool.h"
#include "../headers/EventSystem.h"
#include "../headers/GameStatistics.h"
#include "../headers/Map.h"
#include "../headers/Server.h"
#include "../headers/TankSpawner.h"
#include "../headers/pawns/Bullet.h"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <memory>

//#ifdef _WIN32
//#define _WIN32_WINNT 0x0A00
//#endif
#define ASIO_STANDALONE
#include <boost/asio/io_service.hpp>

std::ofstream error_log_server("error_log_Server.txt");

GameSuccess::GameSuccess(UPoint windowSize, std::shared_ptr<int[]> windowBuffer, std::shared_ptr<SDL_Renderer> renderer,
                         std::shared_ptr<SDL_Texture> screen, std::shared_ptr<TTF_Font> fpsFont,
                         std::shared_ptr<EventSystem> events,
                         std::shared_ptr<GameStatistics> statistics, std::shared_ptr<Menu> menu)
	: _windowSize{windowSize},
	  _statistics{std::move(statistics)},
	  _menu{std::move(menu)},
	  _windowBuffer{windowBuffer},
	  _renderer{std::move(renderer)},
	  _screen{std::move(screen)},
	  _fpsFont{std::move(fpsFont)},
	  _events{events},
	  _bulletPool{std::make_shared<BulletPool>(events, &_allObjects, windowSize, windowBuffer)},
	  _bonusSpawner{events, &_allObjects, windowBuffer, windowSize}
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
	_bulletPool->Clear();//TODO: Replace with on event execute
	SetCurrentGameMode(_selectedGameMode);

	_allObjects.clear();
	_allObjects.reserve(1000);

	_statistics->Reset();
	_events->EmitEvent("InitialSpawn");

	//Map creation
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
	memset(_windowBuffer.get(), 0, size);
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

			const std::unique_ptr<SDL_Surface, void(*)(SDL_Surface*)> fpsSurface(
					TTF_RenderText_Solid(_fpsFont.get(), std::to_string(fps).c_str(), SDL_Color{140, 0, 255, 0}),
					SDL_FreeSurface);
			if (fpsSurface)
			{
				_fpsTexture = std::shared_ptr<SDL_Texture>(
						SDL_CreateTextureFromSurface(_renderer.get(), fpsSurface.get()),
						SDL_DestroyTexture);
			}
		}
		fpsPrevUpdateTime = newTime;
		frameCount = 0;
	}
}

void GameSuccess::UserInputHandling()
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
		if (!(*it)->GetIsAlive())
		{
			if (const auto* bullet = dynamic_cast<Bullet*>(it->get()); bullet != nullptr)
			{
				_bulletPool->ReturnBullet(*it);
				it = _allObjects.erase(it);
				continue;
			}
		}
		++it;
	}

	const auto it = std::ranges::remove_if(_allObjects, [](const auto& obj) { return !obj->GetIsAlive(); }).begin();
	_allObjects.erase(it, _allObjects.end());
}

//TODO: recheck rule of 3/5 for all classes
//TODO: convert enum to enum classes

void GameSuccess::MainLoop()
{
	if (!_windowBuffer || !_renderer || !_screen || !_fpsFont || !_events)
	{
		return;
	}

	try
	{
		Uint32 frameCount{0};
		Uint32 fps{0};
		float deltaTime{0.f};
		Uint64 oldTime = SDL_GetTicks64();
		auto fpsPrevUpdateTime = oldTime;
		const SDL_Rect fpsRectangle{.x = static_cast<int>(_windowSize.x) - 80, .y = 20, .w = 40, .h = 40};

		boost::asio::io_service io_service;
		Server server(io_service, "127.0.0.1", "1234", _events);
		//TODO: encapsulate separated thread into server for storing and running io_service
		std::thread netThread([&]()
		{
			try
			{
				io_service.run();
				// io_service.stop();
			}
			catch (std::exception& e)
			{
				error_log_server << "thread " << e.what() << '\n';
			}
			catch (...)
			{
				error_log_server << "thread error ..." << '\n';
			}
		});
		netThread.detach();

		while (!_isGameOver)
		{
			// Cap to 60 FPS
			// SDL_Delay(static_cast<Uint32>(std::floor(16.666f - deltaTime)));

			ClearBuffer();

			// TODO: current mode demo in game fix this
			UserInputHandling();
			if (_menu)
			{
				_menu.get()->Update();//TODO: should be event updateMenu
			}

			if (!_isPause)
			{
				_events->EmitEvent<const float>("TickUpdate", deltaTime);
			}

			DisposeDeadObject();// TODO: replace with event

			_events->EmitEvent("RespawnTanks");

			_events->EmitEvent("Draw");

			_events->EmitEvent("DrawHealthBar");
			// TODO: create and blend separate buff layers(objects, effect, interface)

			_events->EmitEvent("DrawMenuBackground");

			const Uint64 newTime = SDL_GetTicks64();
			deltaTime = static_cast<float>(newTime - oldTime) / 1000.0f;
			HandleFPS(frameCount, fpsPrevUpdateTime, fps, newTime);

			// update screen with buffer
			SDL_UpdateTexture(_screen.get(), nullptr, _windowBuffer.get(), static_cast<int>(_windowSize.x) << 2);
			SDL_RenderCopy(_renderer.get(), _screen.get(), nullptr, nullptr);

			_events->EmitEvent("DrawMenuText");

			// Copy the texture with FPS to the renderer
			SDL_RenderCopy(_renderer.get(), _fpsTexture.get(), nullptr, &fpsRectangle);

			SDL_RenderPresent(_renderer.get());

			oldTime = newTime;
		}

		// if (t.joinable()) {
		// t.join();//TODO: create signal for closing listening server
		// }
	}
	catch (std::exception& e)
	{
		error_log_server << e.what() << '\n';
	}
	catch (...)
	{
		error_log_server << "error ..." << '\n';
	}
}

int GameSuccess::Result() const { return 0; }

GameMode GameSuccess::GetCurrentGameMode() const { return _currentMode; }

void GameSuccess::SetCurrentGameMode(const GameMode newGameMode)
{
	_currentMode = newGameMode;
	_events->EmitEvent<const GameMode>("GameModeChangedTo", _currentMode);
}
