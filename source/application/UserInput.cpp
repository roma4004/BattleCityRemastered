#include "application/UserInput.h"
#include "application/WindowConfig.h"
#include "application/SDL_Config.h"
#include "components/EventSystem.h"
#include "components/events/CoreLifecycleEvents.h"
#include "components/events/GameModeEvents.h"
#include "components/events/InputEvents.h"
#include "components/events/RenderUIEvents.h"
#include "components/events/TimingEvents.h"
#include "enums/GameMode.h"
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_gamepad.h>
#include <algorithm>
#include "utils/Log.h"

namespace
{
//NOTE: SDL3 dropped SDL_NumJoysticks - the count only comes out of the list, which we own and free
[[nodiscard]] int ConnectedJoystickCount()
{
	int count{};
	SDL_free(SDL_GetJoysticks(&count));

	return count;
}
}

UserInput::UserInput(const std::shared_ptr<EventSystem>& events, const WindowConfig& windowConfig,
					 const SDL_Config& sdlConfig)
	: _selectedGameMode{GameMode::Demo}
	, _windowSize{windowConfig.size}
	, _events{events}
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
	//NOTE: SDL3 split SDL_WINDOWEVENT into one event type per reason - RESIZED covers SDL2's SIZE_CHANGED
	const bool isWindowDrag = event.type == SDL_EVENT_WINDOW_MOVED || event.type == SDL_EVENT_WINDOW_RESIZED;
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
	if (ConnectedJoystickCount() > 1)
	{
		const auto isSameId = [instanceId](const std::shared_ptr<SDL_Gamepad>& controller)
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

			//NOTE: announced here and not on every resize step - resizing the window back while the
			//user is still dragging it fights the drag
			_events->EmitEvent(WindowSizeChangedToEvent{.newSize = _windowSize});
		}
	}
}

SDL_Point UserInput::ToLogical(const float windowX, const float windowY) const
{
	float logicalX{};
	float logicalY{};
	SDL_RenderCoordinatesFromWindow(_sdlConfig.renderer.get(), windowX, windowY, &logicalX, &logicalY);

	return SDL_Point{.x = static_cast<int>(logicalX), .y = static_cast<int>(logicalY)};
}

void UserInput::MouseEvents(const SDL_Event& event)
{
	if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN && event.button.button == SDL_BUTTON_LEFT)
	{
		_mouseButtons.MouseLeftButton = true;

		const SDL_Point mouse = ToLogical(event.button.x, event.button.y);
		if (_isMenuDisplayed && SDL_PointInRect(&mouse, &_allTilesRect))
		{
			_events->EmitEvent(EnterEvent{.isPressed = true});
		}

		return;
	}

	if (event.type == SDL_EVENT_MOUSE_BUTTON_UP && event.button.button == SDL_BUTTON_LEFT)
	{
		_mouseButtons.MouseLeftButton = false;
		_events->EmitEvent(EnterEvent{.isPressed = false});

		return;
	}

	if (event.type == SDL_EVENT_MOUSE_MOTION)
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

	switch (event.key.key)
	{
		case SDLK_W:
			_events->EmitEvent(Key(LocalInput(keyboardLeftSideSlot)), MoveUpEvent{.isPressed = isPressed});
			break;
		case SDLK_UP:
			_events->EmitEvent(Key(LocalInput(keyboardRightSideSlot)), MoveUpEvent{.isPressed = isPressed});
			break;
		case SDLK_A:
			_events->EmitEvent(Key(LocalInput(keyboardLeftSideSlot)), MoveLeftEvent{.isPressed = isPressed});
			break;
		case SDLK_LEFT:
			_events->EmitEvent(Key(LocalInput(keyboardRightSideSlot)), MoveLeftEvent{.isPressed = isPressed});
			break;
		case SDLK_S:
			_events->EmitEvent(Key(LocalInput(keyboardLeftSideSlot)), MoveDownEvent{.isPressed = isPressed});
			break;
		case SDLK_DOWN:
			_events->EmitEvent(Key(LocalInput(keyboardRightSideSlot)), MoveDownEvent{.isPressed = isPressed});
			break;
		case SDLK_D:
			_events->EmitEvent(Key(LocalInput(keyboardLeftSideSlot)), MoveRightEvent{.isPressed = isPressed});
			break;
		case SDLK_RIGHT:
			_events->EmitEvent(Key(LocalInput(keyboardRightSideSlot)), MoveRightEvent{.isPressed = isPressed});
			break;
		case SDLK_SPACE:
			_events->EmitEvent(Key(LocalInput(keyboardLeftSideSlot)), FireEvent{.isPressed = isPressed});
			break;
		case SDLK_RCTRL:
			_events->EmitEvent(Key(LocalInput(keyboardRightSideSlot)), FireEvent{.isPressed = isPressed});
			break;
		case SDLK_M:
			if (isPressed == false)
			{
				_events->EmitEvent(MenuReleasedEvent{});
			}
			break;
		case SDLK_P:
			if (isPressed == false)
			{
				_events->EmitEvent(PauseReleasedEvent{});
			}
			break;
		case SDLK_R:
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
	if (event.type == SDL_EVENT_KEY_DOWN)
	{
		KeyboardKeyPressRelease(event, true);
	}
	else if (event.type == SDL_EVENT_KEY_UP)
	{
		KeyboardKeyPressRelease(event, false);
	}
}

void UserInput::GamepadKeyPressRelease(const SDL_Event& event, const bool& isPressed) const
{
	if (ConnectedJoystickCount() > 0)
	{
		const PlayerSlot controllerSlot{ControllerSlotDefiner(event.gbutton.which)};

		switch (event.gbutton.button)
		{
			case SDL_GAMEPAD_BUTTON_SOUTH:
				_events->EmitEvent(Key(LocalInput(controllerSlot)), FireEvent{.isPressed = isPressed});
				break;
			case SDL_GAMEPAD_BUTTON_EAST:
				//NOTE: no listener consumes this yet
				_events->EmitEvent(GamepadButtonEvent{.controllerSlot = controllerSlot,
													  .button = GamepadButton::B,
													  .isPressed = isPressed});
				break;
			case SDL_GAMEPAD_BUTTON_WEST:
				//NOTE: no listener consumes this yet
				_events->EmitEvent(GamepadButtonEvent{.controllerSlot = controllerSlot,
													  .button = GamepadButton::X,
													  .isPressed = isPressed});
				break;
			case SDL_GAMEPAD_BUTTON_NORTH:
				//NOTE: no listener consumes this yet
				_events->EmitEvent(GamepadButtonEvent{.controllerSlot = controllerSlot,
													  .button = GamepadButton::Y,
													  .isPressed = isPressed});
				if (isPressed == false)
				{
					_events->EmitEvent(TabReleasedEvent{});
				}
				break;
			case SDL_GAMEPAD_BUTTON_DPAD_UP:
				_events->EmitEvent(Key(LocalInput(controllerSlot)), MoveUpEvent{.isPressed = isPressed});
				break;
			case SDL_GAMEPAD_BUTTON_DPAD_DOWN:
				_events->EmitEvent(Key(LocalInput(controllerSlot)), MoveDownEvent{.isPressed = isPressed});
				break;
			case SDL_GAMEPAD_BUTTON_DPAD_LEFT:
				_events->EmitEvent(Key(LocalInput(controllerSlot)), MoveLeftEvent{.isPressed = isPressed});
				break;
			case SDL_GAMEPAD_BUTTON_DPAD_RIGHT:
				_events->EmitEvent(Key(LocalInput(controllerSlot)), MoveRightEvent{.isPressed = isPressed});
				break;
			case SDL_GAMEPAD_BUTTON_START:
				if (isPressed == false)
				{
					_events->EmitEvent(MenuReleasedEvent{});
				}
				break;
			case SDL_GAMEPAD_BUTTON_BACK:
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
		case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
		{
			GamepadKeyPressRelease(event, true);
			break;
		}
		case SDL_EVENT_GAMEPAD_BUTTON_UP:
		{
			GamepadKeyPressRelease(event, false);
			break;
		}
		case SDL_EVENT_GAMEPAD_ADDED:
		{
			Log::Info("joysticks: " + std::to_string(ConnectedJoystickCount()));
			ConnectController({SDL_OpenGamepad(event.gdevice.which), SDL_CloseGamepad});
			break;
		}
		case SDL_EVENT_GAMEPAD_REMOVED:
		{
			const SDL_JoystickID instanceId = event.gdevice.which;
			Log::Info("controller removed (instance " + std::to_string(instanceId) + ')');
			DisconnectController(instanceId);
			break;
		}

		default:
			break;
	}
}

//NOTE: SDL fires this for every pixel of a drag, so only the last size is kept - who cares about it
//is told once, when the drag stops
void UserInput::OnWindowResized(const UPoint newSize)
{
	_windowSize = newSize;
}

void UserInput::Update()
{
	SDL_Event event;
	while (SDL_PollEvent(&event))
	{
		if (event.type == SDL_EVENT_QUIT || (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_ESCAPE))
		{
			_isShutdown = true;
		}

		if (event.type == SDL_EVENT_WINDOW_RESIZED)
		{
			OnWindowResized(UPoint{.x = static_cast<unsigned>(event.window.data1),
								   .y = static_cast<unsigned>(event.window.data2)});
		}

		//NOTE: render targets lost their pixels - whoever drew into one has to draw it again
		if (event.type == SDL_EVENT_RENDER_TARGETS_RESET)
		{
			_events->EmitEvent(RenderTargetsResetEvent{});
		}

		//NOTE: the GPU device died and came back - every texture it ever handed out has to be rebuilt
		if (event.type == SDL_EVENT_RENDER_DEVICE_RESET)
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

void UserInput::ConnectController(const std::shared_ptr<SDL_Gamepad>& newController)
{
	const auto isEmpty = [](const std::shared_ptr<SDL_Gamepad>& controller) { return controller == nullptr; };

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
	const auto isSameId = [instanceId](const std::shared_ptr<SDL_Gamepad>& controller)
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
	int joystickCount{};
	SDL_JoystickID* joysticks = SDL_GetJoysticks(&joystickCount);
	if (joysticks == nullptr)
	{
		Log::Error(std::string{"SDL_GetJoysticks Error: "} + SDL_GetError());

		return;
	}

	Log::Detail(std::to_string(joystickCount) + " gamepad(s) connected");

	//NOTE: two seats, so the pads past the second one stay unopened
	constexpr int kMaxControllers{2};
	for (int i = 0; i < joystickCount && i < kMaxControllers; ++i)
	{
		if (SDL_Gamepad* gamepad = SDL_OpenGamepad(joysticks[i]);
			gamepad != nullptr)
		{
			ConnectController({gamepad, SDL_CloseGamepad});
			Log::Info("opened controller " + std::to_string(i + 1) + ": " + SDL_GetGamepadName(gamepad));
		}
	}

	SDL_free(joysticks);
}

bool UserInput::IsSameController(const std::shared_ptr<SDL_Gamepad>& controller, const SDL_JoystickID instanceId)
{
	if (controller == nullptr)
	{
		return false;
	}

	if (SDL_Joystick* joystick = SDL_GetGamepadJoystick(controller.get());
		joystick != nullptr)
	{
		return SDL_GetJoystickID(joystick) == instanceId;
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
