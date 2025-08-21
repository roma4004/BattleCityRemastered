#include "application/UserInput.h"
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

void UserInput::Subscribe()
{
	_events->AddListener("Pause_Status", _name, [this](const bool newPauseStatus)
	{
		this->_isPause = newPauseStatus;
	});
}

void UserInput::Unsubscribe() const
{
	_events->RemoveListener("Pause_Status", _name);
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
	int device_index = event.cdevice.which;
	SDL_GameController* gc = SDL_GameControllerOpen(device_index);
	SDL_Joystick* joy = SDL_GameControllerGetJoystick(gc);
	SDL_JoystickID instance_id = SDL_JoystickInstanceID(joy);
	controllers[instance_id] = gc;
					
	switch (event.type)
	{
		case SDL_CONTROLLERDEVICEADDED:
		{
					
					
			if (SDL_IsGameController(device_index))
			{
				std::cout << "Controller " << std::to_string(instance_id) << " added" << std::endl;
				SDL_Log("Controller connected: %s (instance %d)",SDL_GameControllerName(gc), instance_id);														
						
			}
			break;
		}
		case SDL_CONTROLLERDEVICEREMOVED:
		{
			auto it = controllers.find(instance_id);
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
	switch (event.cbutton.button)
	{
			case SDL_CONTROLLER_BUTTON_A:
			_events->EmitEvent("CB_A_Pressed"); 
			break;
			case SDL_CONTROLLER_BUTTON_B:
			_events->EmitEvent("CB_B_Pressed");
			break;
			case SDL_CONTROLLER_BUTTON_X:
			_events->EmitEvent("CB_X_Pressed");
			break;
			case SDL_CONTROLLER_BUTTON_Y:
			_events->EmitEvent("CB_Y_Pressed");
			break;
			case SDL_CONTROLLER_BUTTON_DPAD_UP:
			_events->EmitEvent("CB_DPAD_UP_Pressed"); 
			break;
			case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
			_events->EmitEvent("CB_DPAD_DOWN_Pressed"); 
			break;
			case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
			_events->EmitEvent("CB_DPAD_LEFT_Pressed"); 
			break;
			case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
			_events->EmitEvent("CB_DPAD_RIGHT_Pressed"); 
			break;
			case SDL_CONTROLLER_BUTTON_START:
			_events->EmitEvent("Start_Pressed");
			break;
			case SDL_CONTROLLER_BUTTON_GUIDE:
			_events->EmitEvent("GUIDE_Pressed");
			break;
			
			default:
			break;
	}
}

void UserInput::GamepadKeyReleased(const SDL_Event& event) const
{
	switch(event.cbutton.button)
	{
			case SDL_CONTROLLER_BUTTON_A:
			_events->EmitEvent("CB_A_Released");
			break;
			case SDL_CONTROLLER_BUTTON_B:
			_events->EmitEvent("CB_B_Released");
			break;
			case SDL_CONTROLLER_BUTTON_X:
			_events->EmitEvent("CB_X_Released");
			break;
			case SDL_CONTROLLER_BUTTON_Y:
			_events->EmitEvent("CB_Y_Released");
			break;
			case SDL_CONTROLLER_BUTTON_DPAD_UP:
			_events->EmitEvent("CB_DPAD_UP_Released");
			break;
			case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
			_events->EmitEvent("CB_DPAD_DOWN_Released");
			break;
			case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
			_events->EmitEvent("CB_DPAD_LEFT_Released");
			break;
			case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
			_events->EmitEvent("CB_DPAD_RIGHT_Released");
			break;
			case SDL_CONTROLLER_BUTTON_START:
			_events->EmitEvent("Start_Pressed");
			break;
			case SDL_CONTROLLER_BUTTON_GUIDE:
			_events->EmitEvent("GUIDE_Pressed");
			break;

			default:
			break;
	}
}

void UserInput::Gamepad2_KeyPressed(const SDL_Event& event) const
{
	switch (event.cbutton.button)
	{
		case SDL_CONTROLLER_BUTTON_A:
			_events->EmitEvent("C2B_A_Pressed"); 
			break;
		case SDL_CONTROLLER_BUTTON_B:
			_events->EmitEvent("C2B_B_Pressed");
			break;
		case SDL_CONTROLLER_BUTTON_X:
			_events->EmitEvent("C2B_X_Pressed");
			break;
		case SDL_CONTROLLER_BUTTON_Y:
			_events->EmitEvent("C2B_Y_Pressed");
			break;
		case SDL_CONTROLLER_BUTTON_DPAD_UP:
			_events->EmitEvent("C2B_DPAD_UP_Pressed"); 
			break;
		case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
			_events->EmitEvent("C2B_DPAD_DOWN_Pressed"); 
			break;
		case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
			_events->EmitEvent("C2B_DPAD_LEFT_Pressed"); 
			break;
		case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
			_events->EmitEvent("C2B_DPAD_RIGHT_Pressed"); 
			break;
		case SDL_CONTROLLER_BUTTON_START:
			_events->EmitEvent("Start_Pressed");
			break;
		case SDL_CONTROLLER_BUTTON_GUIDE:
			_events->EmitEvent("GUIDE_Pressed");
			break;
			
		default:
			break;
	}
}

void UserInput::Gamepad2_KeyReleased(const SDL_Event& event) const
{
	switch(event.cbutton.button)
	{
		case SDL_CONTROLLER_BUTTON_A:
			_events->EmitEvent("C2B_A_Released");
			break;
		case SDL_CONTROLLER_BUTTON_B:
			_events->EmitEvent("C2B_B_Released");
			break;
		case SDL_CONTROLLER_BUTTON_X:
			_events->EmitEvent("C2B_X_Released");
			break;
		case SDL_CONTROLLER_BUTTON_Y:
			_events->EmitEvent("C2B_Y_Released");
			break;
		case SDL_CONTROLLER_BUTTON_DPAD_UP:
			_events->EmitEvent("C2B_DPAD_UP_Released");
			break;
		case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
			_events->EmitEvent("C2B_DPAD_DOWN_Released");
			break;
		case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
			_events->EmitEvent("C2B_DPAD_LEFT_Released");
			break;
		case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
			_events->EmitEvent("C2B_DPAD_RIGHT_Released");
			break;
		case SDL_CONTROLLER_BUTTON_START:
			_events->EmitEvent("Start_Pressed");
			break;
		case SDL_CONTROLLER_BUTTON_GUIDE:
			_events->EmitEvent("GUIDE_Pressed");
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

void UserInput::Gamepad2_Events(const SDL_Event& event) const
{
	if (event.type == SDL_CONTROLLERBUTTONDOWN)
	{
		Gamepad2_KeyPressed(event);
	}
	else if (event.type == SDL_CONTROLLERBUTTONUP)
	{		
		Gamepad2_KeyReleased(event);
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

		WindowsMoveEvents(event);
		MouseEvents(event);
		KeyboardEvents(event);
		GamepadEvents(event);  		 
		
	}

	OnWindowMoveStop();
}

bool UserInput::IsGameOver() const { return _isGameOver; }

bool UserInput::IsPause() const { return _isPause; }
