#include "application/UserInput.h"
#include "application/GameSuccess.h"
#include "components/EventSystem.h"
#include <SDL_events.h>
#include <SDL_gamecontroller.h>
#include <algorithm>
#include <iostream>
#include <ranges>

UserInput::UserInput(const UPoint windowSize, const std::shared_ptr<EventSystem>& events)
	: _windowSize{windowSize}
	, _events{events}
{
	Subscribe();

	InitControllers();
}

UserInput::~UserInput()
{
	Unsubscribe();

	_slotsForController.clear();
}

void UserInput::Subscribe()
{
	_events->AddListener("Pause_Status", _name, [this](const bool newPauseStatus) { this->_isPause = newPauseStatus; });
	_events->AddListener("Tab_Released", _name, [this]() { this->SwapControllers(); });
	_events->AddListener("PreTickUpdate", _name, [this](const double /*deltaTime*/) { this->Update(); });
}

void UserInput::Unsubscribe() const
{
	_events->RemoveListener("Pause_Status", _name);
	_events->RemoveListener("Tab_Released", _name);
	_events->RemoveListener("PreTickUpdate", _name);
}

void UserInput::WindowsMoveEvents(const SDL_Event& event)
{
	if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_MOVED)
	{
		if (!_isMoving)
		{
			_isMoving = true;

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
	if (_isMoving)
	{
		if (std::chrono::system_clock::now() - _lastMoveEventTime > _moveEndDelay)
		{
			_isMoving = false;

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
		// std::cout << "x: " << x << " \t y: " << y << '\n';
		// const int rowSize = env.windowWidth; ???

		if (x < 1 || y < 1
			|| x >= static_cast<Sint32>(_windowSize.x) - 1 && y >= static_cast<Sint32>(_windowSize.y) - 1)
			{}
	}
}

void UserInput::KeyboardKeyPressRelease(const SDL_Event& event, const bool& isPressed) const
{
	const std::string KeyboardLeftSideTag(_areControllersSwapped ? "P2" : "P1");
	const std::string KeyboardRightSideTag(_areControllersSwapped ? "P1" : "P2");
	std::string KeyStateTag{};

	if (event.key.type == SDL_KEYDOWN)
	{
		KeyStateTag = "Pressed";
	}
	else if (event.key.type == SDL_KEYUP)
	{
		KeyStateTag = "Released";
	}
	else
	{
		return;
	}

	switch (event.key.keysym.sym)
	{
		case SDLK_w:
			_events->EmitEvent(KeyboardLeftSideTag + "_Move_Up", isPressed);
			break;
		case SDLK_UP:
			_events->EmitEvent(KeyboardRightSideTag + "_Move_Up", isPressed);
			break;
		case SDLK_a:
			_events->EmitEvent(KeyboardLeftSideTag + "_Move_Left", isPressed);
			break;
		case SDLK_LEFT:
			_events->EmitEvent(KeyboardRightSideTag + "_Move_Left", isPressed);
			break;
		case SDLK_s:
			_events->EmitEvent(KeyboardLeftSideTag + "_Move_Down", isPressed);
			break;
		case SDLK_DOWN:
			_events->EmitEvent(KeyboardRightSideTag + "_Move_Down", isPressed);
			break;
		case SDLK_d:
			_events->EmitEvent(KeyboardLeftSideTag + "_Move_Right", isPressed);
			break;
		case SDLK_RIGHT:
			_events->EmitEvent(KeyboardRightSideTag + "_Move_Right", isPressed);
			break;
		case SDLK_SPACE:
			_events->EmitEvent(KeyboardLeftSideTag + "_Fire", isPressed);
			break;
		case SDLK_RCTRL:
			_events->EmitEvent(KeyboardRightSideTag + "_Fire", isPressed);
			break;
		case SDLK_m:
			_events->EmitEvent("Menu_" + KeyStateTag);
			break;
		case SDLK_p:
			_events->EmitEvent("Pause_" + KeyStateTag);
			break;
		case SDLK_r:
			_events->EmitEvent("Reset_" + KeyStateTag);
			break;
		case SDLK_TAB:
			_events->EmitEvent("Tab_" + KeyStateTag);
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

void UserInput::GamepadKeyPressRelease(const SDL_Event& event,const std::string& KeyStateTag, const bool& isPressed) const
{
	if (SDL_NumJoysticks() > 0)
	{
		const std::string controllerTag{ControllerTagDefiner(event.cdevice.which)};

		switch (event.cbutton.button)
		{
			case SDL_CONTROLLER_BUTTON_A:
				_events->EmitEvent(controllerTag + "_Fire", isPressed);
				break;
			case SDL_CONTROLLER_BUTTON_B:
				_events->EmitEvent(controllerTag + "_B", isPressed);
				break;
			case SDL_CONTROLLER_BUTTON_X:
				_events->EmitEvent(controllerTag + "_X", isPressed);
				break;
			case SDL_CONTROLLER_BUTTON_Y:
				_events->EmitEvent(controllerTag + "_Y", isPressed);
				break;
			case SDL_CONTROLLER_BUTTON_DPAD_UP:
				_events->EmitEvent(controllerTag + "_Move_Up", isPressed);
				break;
			case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
				_events->EmitEvent(controllerTag + "_Move_Down", isPressed);
				break;
			case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
				_events->EmitEvent(controllerTag + "_Move_Left", isPressed);
				break;
			case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
				_events->EmitEvent(controllerTag + "_Move_Right", isPressed);
				break;
			case SDL_CONTROLLER_BUTTON_START:
				_events->EmitEvent("Menu_" + KeyStateTag);
				break;
			case SDL_CONTROLLER_BUTTON_BACK:
				_events->EmitEvent("Pause_" + KeyStateTag);
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
			GamepadKeyPressRelease(event,"Pressed", true);
			break;
		}
		case SDL_CONTROLLERBUTTONUP:
		{
			GamepadKeyPressRelease(event,"Released", false);
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
	std::cout << numConnectedJoysticks << " gamepad/s connected\n";

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
