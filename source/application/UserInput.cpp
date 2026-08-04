#include "application/UserInput.h"
#include "application/GameConfig.h"
#include "application/GameSuccess.h"
#include "components/EventSystem.h"
#include "enums/GameMode.h"
#include <SDL_events.h>
#include <SDL_gamecontroller.h>
#include <algorithm>
#include <iostream>

UserInput::UserInput(const UPoint windowSize, const std::shared_ptr<EventSystem>& events, GameConfig& gameConfig)
	: _selectedGameMode{GameMode::Demo}
	, _windowSize{windowSize}
	, _events{events}
	, _gameConfig{gameConfig}
{
	Subscribe();

	InitControllers();

	_firstMenuMouseTileDefault = {.x = 175, .y = 135, .w = 200, .h = 30};
	InitMouseHoverTiles({});
}

UserInput::~UserInput()
{
	Unsubscribe();

	_slotsForController.clear();
}

void UserInput::Subscribe()
{
	_events->AddListener("Pause_Status", _name, [this](const bool isPause) { this->_isPause = isPause; });
	_events->AddListener("Tab_Released", _name, [this]() { this->SwapControllers(); });
	_events->AddListener("PreTickUpdate", _name, [this](const double /*deltaTime*/) { this->Update(); });
	_events->AddListener("MenuShowed", _name, [this](const bool isDisplayed) { _isMenuDisplayed = isDisplayed; });
	_events->AddListener("MenuPosChanged", _name, [this](const Point& menuPos)
	{
		_allTilesRect = {
				.x = _menuPos.x + _allTilesRectDefault.x,
				.y = _menuPos.y + _allTilesRectDefault.y,
				.w = _allTilesRectDefault.w,
				.h = _allTilesRectDefault.h
		};
		InitMouseHoverTiles(menuPos);
	});
}

void UserInput::Unsubscribe() const { _events->RemoveAllListeners(_name); }

void UserInput::WindowsMoveEvents(const SDL_Event& event)
{
	if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_MOVED)
	{
		if (!_isWindowMoving)
		{
			_isWindowMoving = true;

			if (!_isPause)
			{
				_isPauseBeforeDragNDrop = true;
				_events->EmitEvent("Pause_Status", _isPauseBeforeDragNDrop);
			}
		}

		_lastMoveEventTime = std::chrono::system_clock::now();
	}
}

void UserInput::SwapControllers()
{
	_areControllersSwapped = !_areControllersSwapped;
	std::cout << "Controllers Swap State: " << _areControllersSwapped << "\n";// left while visual label is absent
}

std::string UserInput::ControllerTagDefiner(const SDL_JoystickID instanceId) const
{
	bool isFirst{true};
	if (SDL_NumJoysticks() > 1)
	{
		const auto isSameId = [instanceId](const std::shared_ptr<SDL_GameController>& controller)
		{
			return IsSameController(controller, instanceId);
		};

		if (const auto it = std::ranges::find_if(_slotsForController, isSameId);
			it != _slotsForController.end())
		{
			isFirst = 1 == std::distance(_slotsForController.begin(), it);
		}
	}

	if (isFirst)
	{
		return _areControllersSwapped ? "P2" : "P1";
	}

	return _areControllersSwapped ? "P1" : "P2";
}

void UserInput::OnWindowMoveStop()
{
	if (_isWindowMoving)
	{
		if (std::chrono::system_clock::now() - _lastMoveEventTime > _moveEndDelay)
		{
			_isWindowMoving = false;

			if (_isPauseBeforeDragNDrop)
			{
				_isPauseBeforeDragNDrop = false;
				_events->EmitEvent("Pause_Status", _isPauseBeforeDragNDrop);
			}
		}
	}
}

void UserInput::MouseEvents(const SDL_Event& event)
{
	if (event.button.button == SDL_BUTTON_LEFT)
	{
		if (event.type == SDL_MOUSEBUTTONDOWN)
		{
			_mouseButtons.MouseLeftButton = true;
		}

		if (event.type == SDL_MOUSEBUTTONUP)
		{
			_mouseButtons.MouseLeftButton = false;
			_events->EmitEvent("Enter", true);
		}

		return;
	}

	if (event.type == SDL_MOUSEMOTION)
	{
		const SDL_Point mouse{.x = event.motion.x, .y = event.motion.y};

		if (_isMenuDisplayed
			&& SDL_PointInRect(&mouse, &_allTilesRect))
		{
			for (auto& [rect, gameMode]: _menuTiles)
			{
				if (SDL_PointInRect(&mouse, &rect)
					&& _selectedGameMode != gameMode)
				{
					_selectedGameMode = gameMode;
					_events->EmitEvent("GameModeSelectedWithMouse", _selectedGameMode);
					break;
				}
			}
		}
	}
}

void UserInput::KeyboardKeyPressRelease(const SDL_Event& event, const bool& isPressed) const
{
	const std::string keyboardLeftSideTag(_areControllersSwapped ? "P2" : "P1");
	const std::string keyboardRightSideTag(_areControllersSwapped ? "P1" : "P2");

	switch (event.key.keysym.sym)
	{
		case SDLK_w:
			_events->EmitEvent("Move_Up", Key(keyboardLeftSideTag), isPressed);
			break;
		case SDLK_UP:
			_events->EmitEvent("Move_Up", Key(keyboardRightSideTag), isPressed);
			break;
		case SDLK_a:
			_events->EmitEvent("Move_Left", Key(keyboardLeftSideTag), isPressed);
			break;
		case SDLK_LEFT:
			_events->EmitEvent("Move_Left", Key(keyboardRightSideTag), isPressed);
			break;
		case SDLK_s:
			_events->EmitEvent("Move_Down", Key(keyboardLeftSideTag), isPressed);
			break;
		case SDLK_DOWN:
			_events->EmitEvent("Move_Down", Key(keyboardRightSideTag), isPressed);
			break;
		case SDLK_d:
			_events->EmitEvent("Move_Right", Key(keyboardLeftSideTag), isPressed);
			break;
		case SDLK_RIGHT:
			_events->EmitEvent("Move_Right", Key(keyboardRightSideTag), isPressed);
			break;
		case SDLK_SPACE:
			_events->EmitEvent("Fire", Key(keyboardLeftSideTag), isPressed);
			break;
		case SDLK_RCTRL:
			_events->EmitEvent("Fire", Key(keyboardRightSideTag), isPressed);
			break;
		case SDLK_m:
			if (isPressed == false)
			{
				_events->EmitEvent("Menu_Released");
			}
			break;
		case SDLK_p:
			if (isPressed == false)
			{
				_events->EmitEvent("Pause_Released");
			}
			break;
		case SDLK_r:
			_events->EmitEvent("Reset_", isPressed);
			break;
		case SDLK_TAB:
			if (isPressed == false)
			{
				_events->EmitEvent("Tab_Released");
			}
			break;
		case SDLK_RETURN:
			_events->EmitEvent("Enter", isPressed);
			break;

		default:
			break;
	}
}

void UserInput::KeyboardEvents(const SDL_Event& event) const
{
	if (event.type == SDL_KEYDOWN)
	{
		KeyboardKeyPressRelease(event, true);
	}
	else if (event.type == SDL_KEYUP)
	{
		KeyboardKeyPressRelease(event, false);
	}
}

void UserInput::GamepadKeyPressRelease(const SDL_Event& event, const bool& isPressed) const
{
	if (SDL_NumJoysticks() > 0)
	{
		const std::string controllerTag{ControllerTagDefiner(event.cdevice.which)};

		switch (event.cbutton.button)
		{
			case SDL_CONTROLLER_BUTTON_A:
				_events->EmitEvent("Fire", Key(controllerTag), isPressed);
				break;
			case SDL_CONTROLLER_BUTTON_B:
				_events->EmitEvent(controllerTag + "_B", isPressed);//NOTE: no listener consumes this yet
				break;
			case SDL_CONTROLLER_BUTTON_X:
				_events->EmitEvent(controllerTag + "_X", isPressed);//NOTE: no listener consumes this yet
				break;
			case SDL_CONTROLLER_BUTTON_Y:
				_events->EmitEvent(controllerTag + "_Y", isPressed);//NOTE: no listener consumes this yet
				if (isPressed == false)
				{
					_events->EmitEvent("Tab_Released");
				}
				break;
			case SDL_CONTROLLER_BUTTON_DPAD_UP:
				_events->EmitEvent("Move_Up", Key(controllerTag), isPressed);
				break;
			case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
				_events->EmitEvent("Move_Down", Key(controllerTag), isPressed);
				break;
			case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
				_events->EmitEvent("Move_Left", Key(controllerTag), isPressed);
				break;
			case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
				_events->EmitEvent("Move_Right", Key(controllerTag), isPressed);
				break;
			case SDL_CONTROLLER_BUTTON_START:
				if (isPressed == false)
				{
					_events->EmitEvent("Menu_Released");
				}
				break;
			case SDL_CONTROLLER_BUTTON_BACK:
				if (isPressed == false)
				{
					_events->EmitEvent("Pause_Released");
				}
				break;

			default:
				break;
		}
	}
}

void UserInput::GamepadEvents(const SDL_Event& event)
{
	switch (event.type)
	{
		case SDL_CONTROLLERBUTTONDOWN:
		{
			GamepadKeyPressRelease(event, true);
			break;
		}
		case SDL_CONTROLLERBUTTONUP:
		{
			GamepadKeyPressRelease(event, false);
			break;
		}
		case SDL_CONTROLLERDEVICEADDED:
		{
			std::cout << "NumJoysticks " << SDL_NumJoysticks() << " \n";
			ConnectController({SDL_GameControllerOpen(event.cdevice.which), SDL_GameControllerClose});
			break;
		}
		case SDL_CONTROLLERDEVICEREMOVED:
		{
			const SDL_JoystickID instanceId = event.cdevice.which;
			std::cout << "Controller removed! (instance " << instanceId << ")\n";
			DisconnectController(instanceId);
			break;
		}

		default:
			break;
	}
}

void UserInput::Update()
{
	SDL_Event event;
	while (SDL_PollEvent(&event))
	{
		if (event.type == SDL_QUIT || (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE))
		{
			_isShutdown = true;
		}

		//TODO: WIP, need scale for game objects and shift pos after winSizeChange
		// if (event.window.event == SDL_WINDOWEVENT_RESIZED || event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
		// {
		// 	constexpr int step{50};
		// 	const auto newWidth = static_cast<unsigned int>(event.window.data1);
		// 	const auto newHeight = static_cast<unsigned int>(event.window.data2);
		// 	const auto snappedWidth = static_cast<unsigned int>(std::round(newWidth / step)) * step;
		// 	const auto snappedHeight = static_cast<unsigned int>(std::round(newHeight / step)) * step;
		// 	const UPoint point{.x = snappedWidth, .y = snappedHeight};
		// 	_events->EmitEvent("WindowSizeChangedTo", point);
		// }
		WindowsMoveEvents(event);
		MouseEvents(event);
		KeyboardEvents(event);
		GamepadEvents(event);
	}

	OnWindowMoveStop();
}

bool UserInput::IsShutdown() const { return _isShutdown; }

bool UserInput::IsPause() const { return _isPause; }

void UserInput::ConnectController(const std::shared_ptr<SDL_GameController>& newController)
{
	const auto isEmpty = [](const std::shared_ptr<SDL_GameController>& controller) { return controller == nullptr; };

	if (const auto it = std::ranges::find_if(_slotsForController, isEmpty);
		it != _slotsForController.end())
	{
		*it = newController;
	}
	else
	{
		_slotsForController.push_back(newController);
	}
}

void UserInput::DisconnectController(const SDL_JoystickID instanceId)
{
	const auto isSameId = [instanceId](const std::shared_ptr<SDL_GameController>& controller)
	{
		return IsSameController(controller, instanceId);
	};

	if (const auto it = std::ranges::find_if(_slotsForController, isSameId);
		it != _slotsForController.end())
	{
		it->reset();
	}
}

void UserInput::InitControllers()
{
	const int numConnectedJoysticks = SDL_NumJoysticks();
	// std::cout << numConnectedJoysticks << " gamepad/s connected\n";

	if (numConnectedJoysticks > 0)
	{
		if (SDL_GameController* GameControllerOne = SDL_GameControllerOpen(0);
			GameControllerOne != nullptr)
		{
			ConnectController({GameControllerOne, SDL_GameControllerClose});
			std::cout << "Opened controller one: " << SDL_GameControllerName(GameControllerOne) << "\n";
		}
	}

	if (numConnectedJoysticks > 1)
	{
		if (SDL_GameController* GameControllerTwo = SDL_GameControllerOpen(1);
			GameControllerTwo != nullptr)
		{
			ConnectController({GameControllerTwo, SDL_GameControllerClose});
			std::cout << "Opened controller two: " << SDL_GameControllerName(GameControllerTwo) << "\n";
		}
	}
}

bool UserInput::IsSameController(const std::shared_ptr<SDL_GameController>& controller, const SDL_JoystickID instanceId)
{
	if (controller == nullptr)
	{
		return false;
	}

	if (SDL_Joystick* joystick = SDL_GameControllerGetJoystick(controller.get());
		joystick != nullptr)
	{
		return SDL_JoystickInstanceID(joystick) == instanceId;
	}

	return false;
}

void UserInput::InitMouseHoverTiles(const Point menuPos)
{
	auto [x, y, w, h] = SDL_Rect{
			.x = menuPos.x + _firstMenuMouseTileDefault.x,
			.y = menuPos.y + _firstMenuMouseTileDefault.y,
			.w = _firstMenuMouseTileDefault.w,
			.h = _firstMenuMouseTileDefault.h
	};

	_allTilesRect = {.x = x, .y = y, .w = w, .h = h * 5};

	_menuTiles = {
			{.rect = {.x = x, .y = y + h * 0, .w = w, .h = h}, .gameMode = GameMode::OnePlayer},
			{.rect = {.x = x, .y = y + h * 1, .w = w, .h = h}, .gameMode = GameMode::TwoPlayers},
			{.rect = {.x = x, .y = y + h * 2, .w = w, .h = h}, .gameMode = GameMode::CoopWithBot},
			{.rect = {.x = x, .y = y + h * 3, .w = w, .h = h}, .gameMode = GameMode::PlayAsHost},
			{.rect = {.x = x, .y = y + h * 4, .w = w, .h = h}, .gameMode = GameMode::PlayAsClient}
	};
}
