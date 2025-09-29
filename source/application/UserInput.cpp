#include "application/UserInput.h"
#include "application/GameSuccess.h"
#include "components/EventSystem.h"
#include <SDL_events.h>
#include <SDL_log.h>
#include <iostream>

UserInput::UserInput(const UPoint windowSize, std::shared_ptr<EventSystem> events):
	_windowSize{windowSize}, _events{std::move(events)},_areControllersSwapped{_areControllersSwapped=false}
{
	Subscribe();
}

UserInput::~UserInput()
{
	Unsubscribe();
}

inline int UserInput::GetDeviceIndex(const SDL_Event &event) 
{
	int device_index = event.cdevice.which;
	//std::cout<<"UserInput::DeviceIndex = "<< device_index<<"\n"; /* left for debug purpose */
	return device_index;
}

void UserInput::Subscribe()
{
	_events->AddListener("Pause_Status", _name, [this](const bool newPauseStatus)
	{
		this->_isPause = newPauseStatus;
	});
	_events->AddListener("ControllersSwapState", _name, [this](){_areControllersSwapped=false;});
}

void UserInput::Unsubscribe() const
{
	_events->RemoveListener("Pause_Status", _name);
	_events->RemoveListener("ControllersSwapState", _name);
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

void UserInput::GamepadInit(SDL_Event &event)
{
	int device_index = GetDeviceIndex(event);
	SDL_GameController* gameController = SDL_GameControllerOpen(device_index);
	SDL_JoystickID instance_id = device_index;
	controllers[instance_id] = gameController;
	switch (event.type)
	{
		case SDL_CONTROLLERDEVICEADDED:
		{					
			if (SDL_IsGameController(device_index))
			{
				std::cout << "Controller " << std::to_string(instance_id) << " added\n";				
				SDL_Log("Controller connected: %s (instance %d)",SDL_GameControllerName(gameController), instance_id);						
			}
			break;
		}		
		case SDL_CONTROLLERDEVICEREMOVED:  //TODO: find out why this case doesn't work
		{
			auto it = controllers.find(device_index);
			if (it != controllers.end())
			{
				SDL_GameControllerClose(it->second);
				controllers.erase(it);
				SDL_Log("Controller  %d disconnected ", instance_id);
			}
			break;
		}
				
		default: break;
	}		
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
		    && y >= static_cast<Sint32>(_windowSize.y) - 1)
		{
			return;
		}
	}
}

void UserInput::KeyPressed(const SDL_Event& event) const
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
		case SDLK_r:
			_events->EmitEvent("Reset_Released");
			break;
		case SDLK_TAB:
		{
		_events->EmitEvent("Tab_Released");
			if (SDL_NumJoysticks()>1)
			{
				if (_areControllersSwapped)
					_areControllersSwapped=false;
				else
					_areControllersSwapped=true;
				std::cout << "Controllers Swap State: " << _areControllersSwapped << "\n"; // left while visual label is absent
			}
			else {std::cout << "Only one controller occurred. Nothing to Swap!  \n";} // left while visual label is absent
		}			
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

	if (SDL_NumJoysticks()>1)
		{
			if (_areControllersSwapped == false)
			{
				if (GetDeviceIndex(event)==1)
					controllerTag= "CB";
				else 
					controllerTag= "C2B";
			}
			else 
			{
				if (GetDeviceIndex(event)==1)
					controllerTag= "C2B";
				else
					controllerTag= "CB";
			}
		}
	else if (SDL_NumJoysticks()==1&&GetDeviceIndex(event)==0)
		controllerTag= "CB";
	else if (SDL_NumJoysticks()==1&&GetDeviceIndex(event)==1)
		controllerTag= "C2B";
	switch (event.cbutton.button)
	{
			case SDL_CONTROLLER_BUTTON_A:
			_events->EmitEvent(controllerTag + "_A_Pressed");
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
			_events->EmitEvent(controllerTag + "_DPAD_UP_Pressed"); 
			break;
			case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
			_events->EmitEvent(controllerTag + "_DPAD_DOWN_Pressed"); 
			break;
			case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
			_events->EmitEvent(controllerTag + "_DPAD_LEFT_Pressed"); 
			break;
			case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
			_events->EmitEvent(controllerTag + "_DPAD_RIGHT_Pressed"); 
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
	if (SDL_NumJoysticks()>1)
	{
		if (_areControllersSwapped == false)
		{
			if (GetDeviceIndex(event)==1)
				controllerTag= "CB";
			else if (GetDeviceIndex(event)==0)
				controllerTag= "C2B";
		}
		if (_areControllersSwapped == true)
		{
			if (GetDeviceIndex(event)==1)
				controllerTag= "C2B";
			else if (GetDeviceIndex(event)==0)
				controllerTag= "CB";
		}
	}
	else if (SDL_NumJoysticks()==1&&GetDeviceIndex(event)==0)
		controllerTag= "CB";
	switch(event.cbutton.button)
	{
			case SDL_CONTROLLER_BUTTON_A:
			_events->EmitEvent(controllerTag + "_A_Released");
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
			_events->EmitEvent(controllerTag + "_DPAD_UP_Released");
			break;
			case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
			_events->EmitEvent(controllerTag + "_DPAD_DOWN_Released");
			break;
			case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
			_events->EmitEvent(controllerTag + "_DPAD_LEFT_Released");
			break;
			case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
			_events->EmitEvent(controllerTag + "_DPAD_RIGHT_Released");
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
	{		GamepadKeyPressed(event);	}
	else if (event.type == SDL_CONTROLLERBUTTONUP)
	{		GamepadKeyReleased(event);	}
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

		WindowsMoveEvents(event);
		MouseEvents(event);
		KeyboardEvents(event);
		GamepadEvents(event);
	}
	OnWindowMoveStop();
}

bool UserInput::IsGameOver() const { return _isGameOver; }

bool UserInput::IsPause() const { return _isPause; }
