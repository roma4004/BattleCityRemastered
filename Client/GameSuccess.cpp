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
	  _userInput{window, events},
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
}

void GameSuccess::Unsubscribe() const
{
	_events->RemoveListener("PreviousGameMode", _name);
	_events->RemoveListener("NextGameMode", _name);
	_events->RemoveListener("ResetBattlefield", _name);
}

void GameSuccess::ResetBattlefield(const GameMode gameMode)
{
	_allObjects.clear();
	_allObjects.reserve(1000);

	SetCurrentGameMode(gameMode);
	if (gameMode == PlayAsClient || gameMode == PlayAsHost)
	{
		_events->EmitEvent("Pause_Released");
	}

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

		while (!_userInput.IsGameOver())
		{
			_window->ClearBuffer();

			_userInput.Update();

			if (_menu)
			{
				_menu->Update();//TODO: replace with event
			}

			if (!_userInput.IsPause() && _currentMode != PlayAsClient)
			{
				_events->EmitEvent<const float>("TickUpdate", deltaTime);
			}

			DisposeDeadObject();

			_events->EmitEvent("RespawnTanks");

			_events->EmitEvent("Draw");

			_events->EmitEvent("DrawHealthBar");// TODO: blend separate buff layers(objects, effect, interface)

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
