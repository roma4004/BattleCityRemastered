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
	std::cout << "Controllers Swap State: " << _areControllersSwapped << "\n";// left while visual label is absent
}

std::string UserInput::ControllerTagDefiner(const SDL_Event& event) const
{
	std::string controllerTag{};
	//std::cout << "Num of pads: " << SDL_NumJoysticks() << "\n"; // Debug
	if (SDL_NumJoysticks() > 1)
	{
		int controllerIndex = GetDeviceIndex(event);
		if (controllerIndex > 1)
		{
			std::cout<<"Pad reconnected with WRONG ID = " << GetDeviceIndex(event)<<" \n";
			controllerIndex = static_cast<int>(_removedID);
		}
		if (!_areControllersSwapped)
		{
			if (controllerIndex == 1)
			{
				controllerTag = "P1";
			}
			else if (controllerIndex == 0)
			{
				controllerTag = "P2";
			}
			else if (controllerIndex > 1)
			{
				_removedID? controllerTag = "P1" : controllerTag = "P2"; 
				//TODO: need to define which from 2 controllers is active to distinguish them 
				//std::cout<<"Pad reconnected with NEW ID = " << GetDeviceIndex(event)<<" \n"; //Debug
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
	else if (SDL_NumJoysticks() == 1)
	{
		controllerTag = "P1";
	}
	//std::cout << "Controller Tag: " << controllerTag << " \n"; //Debug
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

void UserInput::KeyboardKeyPressRelease(const SDL_Event& event) const
{
	std::string KeyboardLeftSideTag {};
	std::string KeyboardRightSideTag {};
	std::string KeyStateTag {};
	if (!_areControllersSwapped)
	{
		KeyboardLeftSideTag = "P1";
		KeyboardRightSideTag = "P2";
	}
	else
	{
		KeyboardLeftSideTag = "P2";
		KeyboardRightSideTag = "P1";
	}

	if (event.key.type == SDL_KEYDOWN)
	{
		KeyStateTag = "Pressed";
	}
	else
	{
		KeyStateTag = "Released";
	}

	switch (event.key.keysym.sym)
	{
		case SDLK_w:
			_events->EmitEvent(KeyboardLeftSideTag + "_Move_Up_" + KeyStateTag);
			break;
		case SDLK_UP:
			_events->EmitEvent(KeyboardRightSideTag + "_Move_Up_" + KeyStateTag);
			break;
		case SDLK_a:
			_events->EmitEvent(KeyboardLeftSideTag + "_Move_Left_" + KeyStateTag);
			break;
		case SDLK_LEFT:
			_events->EmitEvent(KeyboardRightSideTag + "_Move_Left_" + KeyStateTag);
			break;
		case SDLK_s:
			_events->EmitEvent(KeyboardLeftSideTag + "_Move_Down_" + KeyStateTag);
			break;
		case SDLK_DOWN:
			_events->EmitEvent(KeyboardRightSideTag + "_Move_Down_" + KeyStateTag);
			break;
		case SDLK_d:
			_events->EmitEvent(KeyboardLeftSideTag + "_Move_Right_" + KeyStateTag);
			break;
		case SDLK_RIGHT:
			_events->EmitEvent(KeyboardRightSideTag + "_Move_Right_" + KeyStateTag);
			break;
		case SDLK_SPACE:
			_events->EmitEvent(KeyboardLeftSideTag + "_Fire_" + KeyStateTag);
			break;
		case SDLK_RCTRL:
			_events->EmitEvent(KeyboardRightSideTag + "_Fire_" + KeyStateTag);
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
			_events->EmitEvent("Enter_" + KeyStateTag);
			break;

		default:
			break;
	} 
}

void UserInput::KeyboardEvents(const SDL_Event& event) const
{
	if (event.type == SDL_KEYDOWN)
	{
		KeyboardKeyPressRelease(event);
	}
	else if (event.type == SDL_KEYUP)
	{
		KeyboardKeyPressRelease(event);
	}
}

void UserInput::GamepadKeyPressRelease(const SDL_Event& event) const
{
	std::string controllerTag{};
	std::string KeyStateTag {};
	controllerTag = ControllerTagDefiner(event);
	std::cout << "Pressed Tag: " << controllerTag << "\n";
	if (event.cbutton.type == SDL_CONTROLLERBUTTONDOWN)
	{
		KeyStateTag = "Pressed";
	}
	else
	{
		KeyStateTag = "Released";
	}

	switch (event.cbutton.button)
	{
		case SDL_CONTROLLER_BUTTON_A:
			_events->EmitEvent(controllerTag + "_Fire_" + KeyStateTag);
			break;
		case SDL_CONTROLLER_BUTTON_B:
			_events->EmitEvent(controllerTag + "_B_" + KeyStateTag);
			break;
		case SDL_CONTROLLER_BUTTON_X:
			_events->EmitEvent(controllerTag + "_X_" + KeyStateTag);
			break;
		case SDL_CONTROLLER_BUTTON_Y:
			_events->EmitEvent(controllerTag + "_Y_" + KeyStateTag);
			break;
		case SDL_CONTROLLER_BUTTON_DPAD_UP:
			_events->EmitEvent(controllerTag + "_Move_Up_" + KeyStateTag);
			break;
		case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
			_events->EmitEvent(controllerTag + "_Move_Down_" + KeyStateTag);
			break;
		case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
			_events->EmitEvent(controllerTag + "_Move_Left_" + KeyStateTag);
			break;
		case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
			_events->EmitEvent(controllerTag + "_Move_Right_" + KeyStateTag);
			break;
		case SDL_CONTROLLER_BUTTON_START:
			_events->EmitEvent(controllerTag + "_Start_" + KeyStateTag);
			break;
		case SDL_CONTROLLER_BUTTON_GUIDE:
			_events->EmitEvent(controllerTag + "_GUIDE_" + KeyStateTag);
			break;

		default:
			break;
	}
}

void UserInput::GamepadEvents(const SDL_Event& event) const
{
	if (event.type == SDL_CONTROLLERBUTTONDOWN)
	{
		GamepadKeyPressRelease(event);
	}
	else if (event.type == SDL_CONTROLLERBUTTONUP)
	{
		GamepadKeyPressRelease(event);
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
			if (deviceIndex <=1)
			{
				std::cout << "Controller " << std::to_string(instanceID) << " added\n";
				SDL_Log("Controller connected: %s (instance %d)", SDL_GameControllerName(gameController), instanceID);
			}
			else
			{
				if (controllers[0] != nullptr)
				{
					instanceID = 1;
					std::cout << "Controller " << std::to_string(instanceID) << " added\n";
					SDL_Log("Controller connected: %s (instance %d)", SDL_GameControllerName(gameController), instanceID);
				}
				else
				{
					instanceID = 0;
					std::cout << "Controller " << std::to_string(instanceID) << " added\n";
					SDL_Log("Controller connected: %s (instance %d)", SDL_GameControllerName(gameController), instanceID);
				}
			}
		}
		break;
		}
		case SDL_CONTROLLERDEVICEREMOVED: 
		{
			if (instanceID == 1)
			{
				_removedID = true;
			}
			else
			{
				_removedID = false;
			}
			SDL_Log("Controller %s disconnected! (instance %d) ", SDL_GameControllerName(gameController), instanceID);
			
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
