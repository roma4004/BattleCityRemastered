#include "application/UserInput.h"
#include "application/GameSuccess.h"
#include "components/EventSystem.h"
#include <SDL_events.h>
#include <SDL_gamecontroller.h>
#include <algorithm>
#include <ranges>
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

void UserInput::SwapControllers()
{
	_areControllersSwapped = !_areControllersSwapped;
	std::cout << "Controllers Swap State: " << _areControllersSwapped << "\n";// left while visual label is absent
}

std::string UserInput::ControllerTagDefiner(const SDL_Event& event) const
{
	std::string player1Tag{_areControllersSwapped ? "P2" : "P1"};
	std::string player2Tag{_areControllersSwapped ? "P1" : "P2"};

	if (SDL_NumJoysticks() > 1)
	{
		const SDL_JoystickID instanceId = event.cdevice.which;
		const auto it = std::ranges::find_if(_slotsForController, [instanceId](const std::shared_ptr<SDL_GameController>& n) 
				{ return n && SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(n.get())) == instanceId; });

		if (it != _slotsForController.end())
		{
			const auto distance = std::distance(_slotsForController.begin(), it);
			return distance == 1 ? player1Tag : player2Tag;
		}
	}

	return player1Tag;
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
	std::string KeyboardLeftSideTag (_areControllersSwapped? "P2" : "P1");
	std::string KeyboardRightSideTag (_areControllersSwapped? "P1" : "P2");
	std::string KeyStateTag {};

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
	if (event.type == SDL_KEYDOWN || event.type == SDL_KEYUP)
	{
		KeyboardKeyPressRelease(event);
	}

}

void UserInput::GamepadKeyPressRelease(const SDL_Event& event, const std::string& KeyStateTag) const
{
	if (SDL_NumJoysticks()>0)
	{
		std::string controllerTag{ControllerTagDefiner(event)};

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
}

void UserInput::GamepadEvents(const SDL_Event& event)
{
	switch (event.type)
	{
	case SDL_CONTROLLERBUTTONDOWN:
	{
		GamepadKeyPressRelease(event, "Pressed");
		break;
	}
	case SDL_CONTROLLERBUTTONUP:
	{
		GamepadKeyPressRelease(event, "Released");
		break;
	}
	case SDL_CONTROLLERDEVICEADDED:
	{
		std::cout << "NumJoysticks " << SDL_NumJoysticks() << " \n";
		std::shared_ptr<SDL_GameController> newController{SDL_GameControllerOpen(event.cdevice.which), SDL_GameControllerClose};
		const auto it = std::ranges::find_if(_slotsForController,
				[](const auto& n){return n == nullptr;});
		if (it != _slotsForController.end())
		{
			*it = newController;
		}
		else
		{
			_slotsForController.push_back(newController);
		}

		break;
	}
	case SDL_CONTROLLERDEVICEREMOVED: 
	{
		SDL_JoystickID instanceId = event.cdevice.which;
		SDL_Log("Controller removed! (instance %d) ", event.cdevice.which);

		const auto it = std::ranges::find_if(_slotsForController, [instanceId](const auto& n) 
			{ return n && SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(n.get())) == instanceId; });

		if (it != _slotsForController.end())
		{
			it->reset();
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

		WindowsMoveEvents(event);
		MouseEvents(event);
		KeyboardEvents(event);
		GamepadEvents(event);
	}

	OnWindowMoveStop();
}

bool UserInput::IsGameOver() const { return _isGameOver; }

bool UserInput::IsPause() const { return _isPause; }
