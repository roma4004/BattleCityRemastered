#include "../Client/GameSuccess.h"
#include "../headers/BulletPool.h"
#include "../headers/Map.h"
#include "../headers/Menu.h"

#include <algorithm>
#include <iostream>
#include <memory>
#include <boost/asio.hpp>
#include <boost/archive/text_oarchive.hpp>

struct Server;

GameSuccess::GameSuccess(std::shared_ptr<Window> window, std::shared_ptr<SDL_Renderer> renderer,
                         std::shared_ptr<SDL_Texture> screen, std::shared_ptr<TTF_Font> fpsFont,
                         std::shared_ptr<EventSystem> events, std::shared_ptr<GameStatistics> statistics,
                         std::unique_ptr<Menu> menu)
	: _menu{std::move(menu)},
	  _statistics{std::move(statistics)},
	  _events{events},
	  _bulletPool{std::make_shared<BulletPool>(events, &_allObjects, window)},
	  _window{window},
	  _renderer{std::move(renderer)},
	  _screen{std::move(screen)},
	  _fpsFont{std::move(fpsFont)},
	  _bonusSpawner{events, &_allObjects, window},
	  _obstacleSpawner{events, &_allObjects, window}
{
	_tankSpawner = std::make_shared<TankSpawner>(window, &_allObjects, events, _bulletPool);

	Subscribe();

	ResetBattlefield(Demo);
}

GameSuccess::~GameSuccess()
{
	Unsubscribe();
}

void GameSuccess::Subscribe()
{
	_events->AddListener("PreviousGameMode", _name, [this]() { this->PrevGameMode(); });
	_events->AddListener("NextGameMode", _name, [this]() { this->NextGameMode(); });
	_events->AddListener("ResetBattlefield", _name, [this]() { this->ResetBattlefield(this->_selectedGameMode); });
	_events->AddListener<const bool>("Pause_Status", _name, [this](const bool newPauseStatus)
	{
		this->_isPause = newPauseStatus;
	});
}

void GameSuccess::Unsubscribe() const
{
	_events->RemoveListener("PreviousGameMode", _name);
	_events->RemoveListener("NextGameMode", _name);
	_events->RemoveListener("ResetBattlefield", _name);
	_events->RemoveListener<const bool>("Pause_Status", _name);
}

void GameSuccess::ResetBattlefield(const GameMode gameMode)
{
	_allObjects.clear();
	_allObjects.reserve(1000);

	SetCurrentGameMode(gameMode);
	_events->EmitEvent("Reset");

	//Map creation
	const float gridOffset = static_cast<float>(_window->size.y) / 50.f;
	const Map field{&_obstacleSpawner};
	field.MapCreation(gridOffset);
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
	const auto size = _window->size.x * _window->size.y * sizeof(int);
	memset(_window->buffer.get(), 0, size);
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
		    x >= static_cast<Sint32>(_window->size.x) - 1 && y >= static_cast<Sint32>(_window->size.y) - 1)
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
	const auto it = std::ranges::remove_if(_allObjects, [&](const auto& obj) { return !obj->GetIsAlive(); }).
			begin();
	_allObjects.erase(it, _allObjects.end());
}

void GameSuccess::MainLoop()
{
	try
	{
		boost::asio::io_context ioContext;
		boost::asio::ip::tcp::socket socket(ioContext);
		// socket.connect(boost::asio::ip::tcp::endpoint(boost::asio::ip::address::from_string("127.0.0.1"), 1234));
		Client client(ioContext, "127.0.0.1", "1234", _events);
		std::thread io_service_thread([&ioContext]() { ioContext.run(); });
		io_service_thread.detach();

		Uint32 frameCount{0};
		Uint32 fps{0};
		float deltaTime{0.f};
		Uint64 oldTime = SDL_GetTicks64();
		auto fpsPrevUpdateTime = oldTime;
		const SDL_Rect fpsRectangle{.x = static_cast<int>(_window->size.x) - 80, .y = 20, .w = 40, .h = 40};

		while (!_isGameOver)
		{
			ClearBuffer();

			UserInputHandling();

			if (_menu)
			{
				_menu->Update();//TODO: replace with event
			}

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
			SDL_UpdateTexture(_screen.get(), nullptr, _window->buffer.get(), static_cast<int>(_window->size.x) << 2);
			SDL_RenderCopy(_renderer.get(), _screen.get(), nullptr, nullptr);

			_events->EmitEvent("DrawMenuText");

			// Copy the texture with FPS to the renderer
			SDL_RenderCopy(_renderer.get(), _fpsTexture.get(), nullptr, &fpsRectangle);

			SDL_RenderPresent(_renderer.get());

			oldTime = newTime;
		}

		// if (io_service_thread.joinable()) {
		// io_service_thread.join();
		// }
	}
	catch (std::exception& e)
	{
		std::cerr << e.what() << '\n';
	}
	catch (...)
	{
		std::cerr << "error ..." << '\n';
	}
}

int GameSuccess::Result() const { return 0; }

GameMode GameSuccess::GetCurrentGameMode() const { return _currentMode; }

void GameSuccess::SetCurrentGameMode(const GameMode selectedGameMode)
{
	_currentMode = selectedGameMode;
	_events->EmitEvent<const GameMode>("GameModeChangedTo", _currentMode);
}
