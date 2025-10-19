#include "application/UserInput.h"
#include "application/GameSuccess.h"
#include "components/EventSystem.h"
#include <SDL_events.h>
#include <SDL_log.h>
#include <iostream>

UserInput::UserInput(const UPoint windowSize, std::shared_ptr<EventSystem> events)
	: _windowSize{windowSize}, _events{std::move(events)}
{
	Subscribe();
}

UserInput::~UserInput()
{
	Unsubscribe();
}

inline int UserInput::GetDeviceIndex(const SDL_Event& event)
{
	int deviceIndex = event.cdevice.which;
	//std::cout<<"UserInput::DeviceIndex = "<< device_index<<"\n"; /* left for debug purpose */
	return deviceIndex;
}

void UserInput::Subscribe()
{
	_events->AddListener("Pause_Status", _name, [this](const bool newPauseStatus)
	{
		this->_isPause = newPauseStatus;
	});
	_events->AddListener("Tab_Released", _name, [this]() { SwapControllers(); });
}

void UserInput::Unsubscribe() const
{
	_events->RemoveListener("Pause_Status", _name);
	_events->RemoveListener("Tab_Released", _name);
}

void UserInput::WindowsMoveEvents(const SDL_Event& event)
{
	//TODO: if already pause not to pause window again on start dragging
	if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_MOVED)
	{
		if (!_isMoving)
		{
			_isMoving = true;

			if (!_isPause)
			{
				_isPauseBeforeDragNDrop = _isPause;//Backup to let pause status the same as it was before dragging
				_events->EmitEvent("Pause_Released");
			}
		}

		_lastMoveEventTime = std::chrono::system_clock::now();
	}
}

void UserInput::GamepadInit(SDL_Event& event)
{
	int deviceIndexLocal = GetDeviceIndex(event);
	SDL_GameController* gameController = SDL_GameControllerOpen(deviceIndexLocal);
	SDL_JoystickID instanceID = deviceIndexLocal;
	controllers[instanceID] = gameController;
	switch (event.type)
	{
		case SDL_CONTROLLERDEVICEADDED:
		{
			if (SDL_IsGameController(deviceIndex))
			{
				std::cout << "Init->Controller " << std::to_string(instanceID) << " added\n";
				SDL_Log("Controller connected: %s (instance %d)", SDL_GameControllerName(gameController), instanceID);
			}
			break;
		}
		default:
			break;
	}
}

void UserInput::SwapControllers()
{
	_areControllersSwapped = !_areControllersSwapped;
	std::cout << "Controllers Swap State: " << _areControllersSwapped << "\n";
	// left while visual label is absent
}

std::string UserInput::ControllerTagDefiner(const SDL_Event& event) const
{
	int deviceID = GetDeviceIndex(event);
	std::string controllerTag{};
	if (deviceID)
	{
		controllerTag = "P2";
	}
	else
	{
		controllerTag = "P1";
	}

	return controllerTag;
}

void UserInput::OnWindowMoveStop()
{
	if (_isMoving)
	{
		if (std::chrono::system_clock::now() - _lastMoveEventTime > _moveEndDelay)
		{
			_isMoving = false;

			if (!_isPauseBeforeDragNDrop)
			{
				_events->EmitEvent("Pause_Released");
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

		if (x < 1 || y < 1 || x >= static_cast<Sint32>(_windowSize.x) - 1
		    && y >= static_cast<Sint32>(_windowSize.y) - 1) {}
	}
}

void UserInput::KeyPressed(const SDL_Event& event) const
{
	std::string controllerTag = ControllerTagDefiner(event);
	switch (event.key.keysym.sym)
	{
		case SDLK_w:
		case SDL_CONTROLLER_BUTTON_DPAD_UP:
		case SDL_CONTROLLER_BUTTON_PS_DPAD_UP:
			_events->EmitEvent(controllerTag + "_Move_Up_Pressed");
			break;
		case SDLK_a:
		case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
		case SDL_CONTROLLER_BUTTON_PS_DPAD_LEFT:
			_events->EmitEvent(controllerTag + "_Move_Left_Pressed");
			break;
		case SDLK_s:
		case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
		case SDL_CONTROLLER_BUTTON_PS_DPAD_DOWN:
			_events->EmitEvent(controllerTag + "_Move_Down_Pressed");
			break;
		case SDLK_d:
		case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
		case SDL_CONTROLLER_BUTTON_PS_DPAD_RIGHT:
			_events->EmitEvent(controllerTag + "_Move_Right_Pressed");
			break;
		case SDLK_SPACE:
		case SDLK_RCTRL:
		case SDL_CONTROLLER_BUTTON_PS_CROSS:
		case SDL_CONTROLLER_BUTTON_A:
			_events->EmitEvent(controllerTag + "_Fire_Pressed");
			break;
		case SDLK_m:
			_events->EmitEvent("Menu_Pressed");
			break;
		case SDLK_p:
			_events->EmitEvent("Pause_Pressed");
			break;
		case SDLK_r:
			_events->EmitEvent("Reset_Pressed");
			break;
		case SDLK_TAB:
			_events->EmitEvent("Tab_Pressed");
			break;
		default:
			break;
	}
}

void UserInput::KeyReleased(const SDL_Event& event) const
{
	std::string controllerTag = ControllerTagDefiner(event);
	switch (event.key.keysym.sym)
	{
		case SDLK_w:
		case SDL_CONTROLLER_BUTTON_DPAD_UP:
		case SDL_CONTROLLER_BUTTON_PS_DPAD_UP:
			_events->EmitEvent(controllerTag + "_Move_Up_Released");
			break;
		case SDLK_a:
		case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
		case SDL_CONTROLLER_BUTTON_PS_DPAD_LEFT:
			_events->EmitEvent(controllerTag + "_Move_Left_Released");
			break;
		case SDLK_s:
		case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
		case SDL_CONTROLLER_BUTTON_PS_DPAD_DOWN:
			_events->EmitEvent(controllerTag + "_Move_Down_Released");
			break;
		case SDLK_d:
		case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
		case SDL_CONTROLLER_BUTTON_PS_DPAD_RIGHT:
			_events->EmitEvent(controllerTag + "_Move_Right_Released");
			break;
		case SDLK_SPACE:
		case SDLK_RCTRL:
		case SDL_CONTROLLER_BUTTON_A:
		case SDL_CONTROLLER_BUTTON_PS_CROSS:
			_events->EmitEvent(controllerTag + "_Fire_Released");
			break;
		case SDLK_m:
			_events->EmitEvent("Menu_Released");
			break;
		case SDLK_p:
			_events->EmitEvent("Pause_Released");
			break;
		case SDLK_r:
			_events->EmitEvent("Reset_Released");
			break;
		case SDLK_TAB:
			_events->EmitEvent("Tab_Released");
		break;
		default:
	break;
	}
}

void UserInput::KeyboardEvents(const SDL_Event& event) const
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

void UserInput::GamepadKeyPressed(const SDL_Event& event) const
{
	std::string controllerTag{};

	if (SDL_NumJoysticks() > 1)
	{
		if (_areControllersSwapped == false)
	{
		if (GetDeviceIndex(event) == 0)
		{
			controllerTag = "P1";
		}
		else
		{
			controllerTag = "P2";
		}
		}
		else
		{
			if (GetDeviceIndex(event) == 1)
		{
			controllerTag = "P2";
		}
		else
		{
			controllerTag = "P1";
		}
	}
	}
	else if (SDL_NumJoysticks() == 1 && GetDeviceIndex(event) == 0)
	{
		controllerTag = "P1";
	}
	else if (SDL_NumJoysticks() == 1 && GetDeviceIndex(event) == 1)
	{
		controllerTag = "P2";
	}

	switch (event.cbutton.button)
	{
		case SDL_CONTROLLER_BUTTON_A:
			_events->EmitEvent(controllerTag + "_Fire_Pressed");
			break;
		case SDL_CONTROLLER_BUTTON_B:
			_events->EmitEvent(controllerTag + "_B_Pressed");
			break;
		case SDL_CONTROLLER_BUTTON_X:
			_events->EmitEvent(controllerTag + "_X_Pressed");
			break;
		case SDL_CONTROLLER_BUTTON_Y:
			_events->EmitEvent(controllerTag + "_Y_Pressed");
			break;
		case SDL_CONTROLLER_BUTTON_DPAD_UP:
			_events->EmitEvent(controllerTag + "_Move_Up_Pressed");
			break;
		case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
			_events->EmitEvent(controllerTag + "_Move_Down_Pressed");
			break;
		case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
			_events->EmitEvent(controllerTag + "_Move_Left_Pressed");
			break;
		case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
			_events->EmitEvent(controllerTag + "_Move_Right_Pressed");
			break;
		case SDL_CONTROLLER_BUTTON_START:
			_events->EmitEvent(controllerTag + "_Start_Pressed");
			break;
		case SDL_CONTROLLER_BUTTON_GUIDE:
			_events->EmitEvent(controllerTag + "_GUIDE_Pressed");
			break;

		default:
			break;
	}
}

void UserInput::GamepadKeyReleased(const SDL_Event& event) const
{
	std::string controllerTag{};
	bool isFirst =  GetDeviceIndex(event) == 0;
	bool isSwaped = _areControllersSwapped;
	isFirst = isSwaped? !isFirst : isFirst;

	controllerTag = isFirst? "P1" : "P2";

	//TODO: Replace with method definition for controller tags
	switch (event.cbutton.button)
	{
		case SDL_CONTROLLER_BUTTON_A:
			_events->EmitEvent(controllerTag + "_Fire_Released");
			break;
		case SDL_CONTROLLER_BUTTON_B:
			_events->EmitEvent(controllerTag + "_B_Released");
			break;
		case SDL_CONTROLLER_BUTTON_X:
			_events->EmitEvent(controllerTag + "_X_Released");
			break;
		case SDL_CONTROLLER_BUTTON_Y:
			_events->EmitEvent(controllerTag + "_Y_Released");
			break;
		case SDL_CONTROLLER_BUTTON_DPAD_UP:
			_events->EmitEvent(controllerTag + "_Move_Up_Released");
			break;
		case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
			_events->EmitEvent(controllerTag + "_Move_Down_Released");
			break;
		case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
			_events->EmitEvent(controllerTag + "_Move_Left_Released");
			break;
		case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
			_events->EmitEvent(controllerTag + "_Move_Right_Released");
			break;
		case SDL_CONTROLLER_BUTTON_START:
			_events->EmitEvent(controllerTag + "_Start_Released");
			break;
		case SDL_CONTROLLER_BUTTON_GUIDE:
			_events->EmitEvent(controllerTag + "_GUIDE_Released");
			break;

		default:
			break;
	}
}

void UserInput::GamepadEvents(const SDL_Event& event) const
{
	if (event.type == SDL_CONTROLLERBUTTONDOWN)
	{
		GamepadKeyPressed(event);
	}
	else if (event.type == SDL_CONTROLLERBUTTONUP)
	{
		GamepadKeyReleased(event);
	}
}

void UserInput::GamepadsPlugAndPlay(const SDL_Event& event) 
{
	deviceIndex = GetDeviceIndex(event);
	SDL_GameController* gameController = SDL_GameControllerOpen(deviceIndex);
	SDL_JoystickID instanceID = deviceIndex;

	switch (event.type)
	{
		case SDL_CONTROLLERDEVICEADDED:
		{
			if (SDL_IsGameController(deviceIndex))
			{
				std::cout << "Controller " << std::to_string(instanceID) << " added\n";
				SDL_Log("Controller connected: %s (instance %d)", SDL_GameControllerName(gameController), instanceID);
			}
			break;
		}
		case SDL_CONTROLLERDEVICEREMOVED: 
		{
			SDL_Log("Controller  %d disconnected ", instanceID);
			auto it = controllers.find(deviceIndex);
			if (it != controllers.end())
			{
				SDL_GameControllerClose(it->second);
				controllers.erase(it);
			}
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
			_isGameOver = true;
		}

		GamepadsPlugAndPlay(event);
		WindowsMoveEvents(event);
		MouseEvents(event);
		KeyboardEvents(event);
		GamepadEvents(event);
	}
	OnWindowMoveStop();
}

bool UserInput::IsGameOver() const { return _isGameOver; }

bool UserInput::IsPause() const { return _isPause; }
