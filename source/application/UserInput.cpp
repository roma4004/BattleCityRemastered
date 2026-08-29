#include "application/UserInput.h"
#include "application/GameConfig.h"
#include "application/WindowConfig.h"
#include "application/SDL_Config.h"
#include "components/EventSystem.h"
#include "components/events/CoreLifecycleEvents.h"
#include "components/events/GameModeEvents.h"
#include "components/events/InputEvents.h"
#include "components/events/RenderUIEvents.h"
#include "components/events/TimingEvents.h"
#include "enums/GameMode.h"
#include <SDL_events.h>
#include <SDL_gamecontroller.h>
#include <algorithm>
#include "utils/Log.h"

UserInput::UserInput(const std::shared_ptr<EventSystem>& events, const GameConfig& gameConfig,
					 const WindowConfig& windowConfig, SDL_Config& sdlConfig)
	: _selectedGameMode{GameMode::Demo}
	, _windowSize{windowConfig.size}
	, _events{events}
	, _gameConfig{gameConfig}
	, _sdlConfig{sdlConfig}
{
	Subscribe();

	InitControllers();

	_firstMenuMouseTileDefault = {.x = 175, .y = 135, .w = 200, .h = 30};
	InitMouseHoverTiles({});
}

UserInput::~UserInput()
{
	_slotsForController.clear();
}

void UserInput::Subscribe()
{
	_subs.push_back(_events->AddListener(this, &UserInput::OnPauseStatus));
	_subs.push_back(_events->AddListener(this, &UserInput::SwapControllers));
	_subs.push_back(_events->AddListener(this, &UserInput::OnPreTickUpdate));
	_subs.push_back(_events->AddListener(this, &UserInput::OnMenuShowed));
	_subs.push_back(_events->AddListener(this, &UserInput::OnMenuPosChanged));
}

void UserInput::OnPauseStatus(const PauseStatusEvent& event) { _isPause = event.isPaused; }

void UserInput::OnPreTickUpdate(const PreTickUpdateEvent&) { Update(); }

void UserInput::OnMenuShowed(const MenuShowedEvent& event) { _isMenuDisplayed = event.isShown; }

void UserInput::OnMenuPosChanged(const MenuPosChangedEvent& event)
{
	_allTilesRect = {
			.x = _menuPos.x + _allTilesRectDefault.x,
			.y = _menuPos.y + _allTilesRectDefault.y,
			.w = _allTilesRectDefault.w,
			.h = _allTilesRectDefault.h
	};
	InitMouseHoverTiles(event.pos);
}

void UserInput::WindowDragEvents(const SDL_Event& event)
{
	const bool isWindowDrag = event.type == SDL_WINDOWEVENT
							  && (event.window.event == SDL_WINDOWEVENT_MOVED
								  || event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED);
	if (!isWindowDrag)
	{
		return;
	}

	if (!_isWindowDragging)
	{
		_isWindowDragging = true;

		if (!_isPause)
		{
			_isPausedByWindowDrag = true;
			_events->EmitEvent(SetPauseEvent{.isPaused = _isPausedByWindowDrag});
		}
	}

	_lastDragEventTime = std::chrono::steady_clock::now();
}

void UserInput::SwapControllers(const TabReleasedEvent&)
{
	_areControllersSwapped = !_areControllersSwapped;
	Log::Info("controllers swap state: " + std::to_string(_areControllersSwapped));// left while visual label is absent
}

PlayerSlot UserInput::ControllerSlotDefiner(const SDL_JoystickID instanceId) const
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
		return _areControllersSwapped ? PlayerSlot::P2 : PlayerSlot::P1;
	}

	return _areControllersSwapped ? PlayerSlot::P1 : PlayerSlot::P2;
}

void UserInput::OnWindowDragStop()
{
	if (_isWindowDragging)
	{
		if (std::chrono::steady_clock::now() - _lastDragEventTime > _dragEndDelay)
		{
			_isWindowDragging = false;

			if (_isPausedByWindowDrag)
			{
				_isPausedByWindowDrag = false;
				_events->EmitEvent(SetPauseEvent{.isPaused = _isPausedByWindowDrag});
			}
		}
	}
}

SDL_Point UserInput::ToLogical(const int windowX, const int windowY) const
{
	float logicalX{};
	float logicalY{};
	SDL_RenderWindowToLogical(_sdlConfig.renderer.get(), windowX, windowY, &logicalX, &logicalY);

	return SDL_Point{.x = static_cast<int>(logicalX), .y = static_cast<int>(logicalY)};
}

void UserInput::MouseEvents(const SDL_Event& event)
{
	if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT)
	{
		_mouseButtons.MouseLeftButton = true;

		const SDL_Point mouse = ToLogical(event.button.x, event.button.y);
		if (_isMenuDisplayed && SDL_PointInRect(&mouse, &_allTilesRect))
		{
			_events->EmitEvent(EnterEvent{.isPressed = true});
		}

		return;
	}

	if (event.type == SDL_MOUSEBUTTONUP && event.button.button == SDL_BUTTON_LEFT)
	{
		_mouseButtons.MouseLeftButton = false;
		_events->EmitEvent(EnterEvent{.isPressed = false});

		return;
	}

	if (event.type == SDL_MOUSEMOTION)
	{
		const SDL_Point mouse = ToLogical(event.motion.x, event.motion.y);

		if (_isMenuDisplayed
			&& SDL_PointInRect(&mouse, &_allTilesRect))
		{
			for (auto& [rect, gameMode]: _menuTiles)
			{
				if (SDL_PointInRect(&mouse, &rect)
					&& _selectedGameMode != gameMode)
				{
					_selectedGameMode = gameMode;
					_events->EmitEvent(SelectedGameModeChangedToEvent{.mode = _selectedGameMode});
					break;
				}
			}
		}
	}
}

void UserInput::KeyboardKeyPressRelease(const SDL_Event& event, const bool& isPressed) const
{
	const PlayerSlot keyboardLeftSideSlot{_areControllersSwapped ? PlayerSlot::P2 : PlayerSlot::P1};
	const PlayerSlot keyboardRightSideSlot{_areControllersSwapped ? PlayerSlot::P1 : PlayerSlot::P2};

	switch (event.key.keysym.sym)
	{
		case SDLK_w:
			_events->EmitEvent(Key(keyboardLeftSideSlot), MoveUpEvent{.isPressed = isPressed});
			break;
		case SDLK_UP:
			_events->EmitEvent(Key(keyboardRightSideSlot), MoveUpEvent{.isPressed = isPressed});
			break;
		case SDLK_a:
			_events->EmitEvent(Key(keyboardLeftSideSlot), MoveLeftEvent{.isPressed = isPressed});
			break;
		case SDLK_LEFT:
			_events->EmitEvent(Key(keyboardRightSideSlot), MoveLeftEvent{.isPressed = isPressed});
			break;
		case SDLK_s:
			_events->EmitEvent(Key(keyboardLeftSideSlot), MoveDownEvent{.isPressed = isPressed});
			break;
		case SDLK_DOWN:
			_events->EmitEvent(Key(keyboardRightSideSlot), MoveDownEvent{.isPressed = isPressed});
			break;
		case SDLK_d:
			_events->EmitEvent(Key(keyboardLeftSideSlot), MoveRightEvent{.isPressed = isPressed});
			break;
		case SDLK_RIGHT:
			_events->EmitEvent(Key(keyboardRightSideSlot), MoveRightEvent{.isPressed = isPressed});
			break;
		case SDLK_SPACE:
			_events->EmitEvent(Key(keyboardLeftSideSlot), FireEvent{.isPressed = isPressed});
			break;
		case SDLK_RCTRL:
			_events->EmitEvent(Key(keyboardRightSideSlot), FireEvent{.isPressed = isPressed});
			break;
		case SDLK_m:
			if (isPressed == false)
			{
				_events->EmitEvent(MenuReleasedEvent{});
			}
			break;
		case SDLK_p:
			if (isPressed == false)
			{
				_events->EmitEvent(PauseReleasedEvent{});
			}
			break;
		case SDLK_r:
			_events->EmitEvent(ResetKeyEvent{.isPressed = isPressed});
			break;
		case SDLK_TAB:
			if (isPressed == false)
			{
				_events->EmitEvent(TabReleasedEvent{});
			}
			break;
		case SDLK_RETURN:
			_events->EmitEvent(EnterEvent{.isPressed = isPressed});
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
		const PlayerSlot controllerSlot{ControllerSlotDefiner(event.cdevice.which)};

		switch (event.cbutton.button)
		{
			case SDL_CONTROLLER_BUTTON_A:
				_events->EmitEvent(Key(controllerSlot), FireEvent{.isPressed = isPressed});
				break;
			case SDL_CONTROLLER_BUTTON_B:
				//NOTE: no listener consumes this yet
				_events->EmitEvent(GamepadButtonEvent{.controllerSlot = controllerSlot,
													  .button = GamepadButton::B,
													  .isPressed = isPressed});
				break;
			case SDL_CONTROLLER_BUTTON_X:
				//NOTE: no listener consumes this yet
				_events->EmitEvent(GamepadButtonEvent{.controllerSlot = controllerSlot,
													  .button = GamepadButton::X,
													  .isPressed = isPressed});
				break;
			case SDL_CONTROLLER_BUTTON_Y:
				//NOTE: no listener consumes this yet
				_events->EmitEvent(GamepadButtonEvent{.controllerSlot = controllerSlot,
													  .button = GamepadButton::Y,
													  .isPressed = isPressed});
				if (isPressed == false)
				{
					_events->EmitEvent(TabReleasedEvent{});
				}
				break;
			case SDL_CONTROLLER_BUTTON_DPAD_UP:
				_events->EmitEvent(Key(controllerSlot), MoveUpEvent{.isPressed = isPressed});
				break;
			case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
				_events->EmitEvent(Key(controllerSlot), MoveDownEvent{.isPressed = isPressed});
				break;
			case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
				_events->EmitEvent(Key(controllerSlot), MoveLeftEvent{.isPressed = isPressed});
				break;
			case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
				_events->EmitEvent(Key(controllerSlot), MoveRightEvent{.isPressed = isPressed});
				break;
			case SDL_CONTROLLER_BUTTON_START:
				if (isPressed == false)
				{
					_events->EmitEvent(MenuReleasedEvent{});
				}
				break;
			case SDL_CONTROLLER_BUTTON_BACK:
				if (isPressed == false)
				{
					_events->EmitEvent(PauseReleasedEvent{});
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
			Log::Info("joysticks: " + std::to_string(SDL_NumJoysticks()));
			ConnectController({SDL_GameControllerOpen(event.cdevice.which), SDL_GameControllerClose});
			break;
		}
		case SDL_CONTROLLERDEVICEREMOVED:
		{
			const SDL_JoystickID instanceId = event.cdevice.which;
			Log::Info("controller removed (instance " + std::to_string(instanceId) + ')');
			DisconnectController(instanceId);
			break;
		}

		default:
			break;
	}
}

//NOTE: SDL fires this for every pixel of a drag, and each one refits the world and rescales every
//object on the field - so the size is snapped to a step and a repeat of the same size is dropped
void UserInput::OnWindowResized(const UPoint newSize)
{
	constexpr std::size_t step{50u};
	const UPoint snapped{.x = std::max(step, (newSize.x + step / 2u) / step * step),
						 .y = std::max(step, (newSize.y + step / 2u) / step * step)};

	if (snapped.x == _windowSize.x && snapped.y == _windowSize.y)
	{
		return;
	}

	_windowSize = snapped;
	_events->EmitEvent(WindowSizeChangedToEvent{.newSize = snapped});
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

		if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
		{
			OnWindowResized(UPoint{.x = static_cast<unsigned>(event.window.data1),
								   .y = static_cast<unsigned>(event.window.data2)});
		}

		//NOTE: render targets lost their pixels - whoever drew into one has to draw it again
		if (event.type == SDL_RENDER_TARGETS_RESET)
		{
			_events->EmitEvent(RenderTargetsResetEvent{});
		}

		//NOTE: the GPU device died and came back - every texture it ever handed out has to be rebuilt
		if (event.type == SDL_RENDER_DEVICE_RESET)
		{
			_events->EmitEvent(RenderDeviceResetEvent{});
		}

		WindowDragEvents(event);
		MouseEvents(event);
		KeyboardEvents(event);
		GamepadEvents(event);
	}

	OnWindowDragStop();
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
	Log::Detail(std::to_string(numConnectedJoysticks) + " gamepad(s) connected");

	if (numConnectedJoysticks > 0)
	{
		if (SDL_GameController* GameControllerOne = SDL_GameControllerOpen(0);
			GameControllerOne != nullptr)
		{
			ConnectController({GameControllerOne, SDL_GameControllerClose});
			Log::Info(std::string{"opened controller one: "} + SDL_GameControllerName(GameControllerOne));
		}
	}

	if (numConnectedJoysticks > 1)
	{
		if (SDL_GameController* GameControllerTwo = SDL_GameControllerOpen(1);
			GameControllerTwo != nullptr)
		{
			ConnectController({GameControllerTwo, SDL_GameControllerClose});
			Log::Info(std::string{"opened controller two: "} + SDL_GameControllerName(GameControllerTwo));
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
