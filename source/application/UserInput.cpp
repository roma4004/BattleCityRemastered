#include "application/UserInput.h"
#include "application/WindowConfig.h"
#include "application/SDL_Config.h"
#include "components/EventSystem.h"
#include "components/events/CoreLifecycleEvents.h"
#include "components/events/GameModeEvents.h"
#include "components/events/InputEvents.h"
#include "components/events/RenderUIEvents.h"
#include "components/events/TimingEvents.h"
#include "enums/Direction.h"
#include "enums/GameMode.h"
#include "enums/PlayerSlot.h"
#include <SDL3/SDL_clipboard.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_gamepad.h>
#include <SDL3/SDL_keyboard.h>
#include <algorithm>
#include <array>
#include <cstddef>
#include <iterator>
#include <optional>
#include <ranges>
#include <string>
#include <utility>
#include <vector>
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

bool IsSameController(const std::shared_ptr<SDL_Gamepad>& controller, const SDL_JoystickID instanceId)
{
	if (controller == nullptr)
	{
		return false;
	}

	if (SDL_Joystick* joystick{SDL_GetGamepadJoystick(controller.get())};
		joystick != nullptr)
	{
		return SDL_GetJoystickID(joystick) == instanceId;
	}

	return false;
}

//NOTE: by the key's place, not the layout - SDL names a non-Latin layout's keys as on QWERTY
[[nodiscard]] std::optional<char> AddressSymbol(const SDL_KeyboardEvent& key)
{
	if ((key.mod & (SDL_KMOD_CTRL | SDL_KMOD_ALT)) != 0)
	{
		return std::nullopt;
	}

	if (key.key >= SDLK_A && key.key <= SDLK_Z)
	{
		return static_cast<char>(key.key);
	}

	const bool isShift{(key.mod & SDL_KMOD_SHIFT) != 0};
	//NOTE: without Num Lock the pad's dot is Delete
	if ((key.key == SDLK_PERIOD && !isShift) || (key.key == SDLK_KP_PERIOD && (key.mod & SDL_KMOD_NUM) != 0))
	{
		return '.';
	}

	if (key.key == SDLK_SEMICOLON && isShift)
	{
		return ':';
	}

	if ((key.key == SDLK_LEFTBRACKET || key.key == SDLK_RIGHTBRACKET) && !isShift)
	{
		return key.key == SDLK_LEFTBRACKET ? '[' : ']';
	}

	return std::nullopt;
}
}// namespace

UserInput::UserInput(const std::shared_ptr<EventSystem>& events, const WindowConfig& windowConfig,
					 const SDL_Config& sdlConfig, const int gamepadDeadZone)
	: _selectedGameMode{GameMode::OnePlayer}
	, _windowSize{windowConfig.size}
	, _events{events}
	, _gamepadDeadZone{gamepadDeadZone}
	, _sdlConfig{sdlConfig}
{
	Subscribe();

	InitControllers();
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
	_subs.push_back(_events->AddListener(this, &UserInput::OnMenuShown));
	_subs.push_back(_events->AddListener(this, &UserInput::OnServerScreenShown));
	_subs.push_back(_events->AddListener(this, &UserInput::OnPanelRowsPlaced));
	_subs.push_back(_events->AddListener(this, &UserInput::OnMenuTilesPlaced));
	_subs.push_back(_events->AddListener(this, &UserInput::OnGameModeChangedTo));
	_subs.push_back(_events->AddListener(this, &UserInput::OnPlayerSlotAssigned));
}

void UserInput::OnPauseStatus(const PauseStatusEvent& event) { _isPause = event.isPaused; }

void UserInput::OnPreTickUpdate(const PreTickUpdateEvent&) { Update(); }

void UserInput::OnMenuShown(const MenuShownEvent& event) { _isMenuShown = event.isShown; }

//NOTE: SDL sends typed text only between these two calls
void UserInput::OnServerScreenShown(const ServerScreenShownEvent& event)
{
	_isTyping = event.isShown;
	_hoveredPanelRow.reset();
	if (_isTyping)
	{
		SDL_StartTextInput(_sdlConfig.sdlWindow.get());
	}
	else
	{
		SDL_StopTextInput(_sdlConfig.sdlWindow.get());
	}
}

void UserInput::OnPanelRowsPlaced(const PanelRowsPlacedEvent& event)
{
	const auto toRect = [&event](const Point& row)
	{
		return SDL_Rect{.x = row.x, .y = row.y, .w = event.rowSize.x, .h = event.rowSize.y};
	};
	_panelRows = event.rows | std::views::transform(toRect) | std::ranges::to<std::vector>();
	_panelSymbolWidth = event.symbolWidth;
}

//NOTE: a new link may hand out another seat
void UserInput::OnGameModeChangedTo(const GameModeChangedToEvent&) { _ownSlot.reset(); }

void UserInput::OnPlayerSlotAssigned(const PlayerSlotAssignedEvent& event) { _ownSlot = event.slot; }

PlayerSlot UserInput::DeviceSlot(const std::size_t index) const
{
	if (index == 0u && _ownSlot && SeatIndex(*_ownSlot) >= 2u)
	{
		return *_ownSlot;
	}

	return SlotForDevice(index, _areControllersSwapped, _isSecondPairSwapped);
}

void UserInput::OnMenuTilesPlaced(const MenuTilesPlacedEvent& event)
{
	static constexpr std::array kModes{GameMode::OnePlayer,  GameMode::TwoPlayers,           GameMode::CoopWithBot,
									   GameMode::FreeForAll, GameMode::TwoPlayersFreeForAll, GameMode::PlayAsHost,
									   GameMode::PlayAsClient};

	_menuTiles.clear();
	for (std::size_t row{}; row < event.tiles.size() && row < kModes.size(); ++row)
	{
		_menuTiles.push_back(SubTile{.rect = {.x = event.tiles[row].x,
											  .y = event.tiles[row].y,
											  .w = event.tileSize.x,
											  .h = event.tileSize.y},
									 .gameMode = kModes[row]});
	}

	_allTilesRect = _menuTiles.empty()
							? SDL_Rect{}
							: SDL_Rect{.x = _menuTiles.front().rect.x,
									   .y = _menuTiles.front().rect.y,
									   .w = event.tileSize.x,
									   .h = event.tileSize.y * static_cast<int>(_menuTiles.size())};
}

void UserInput::WindowDragEvents(const SDL_Event& event)
{
	//NOTE: SDL3 split SDL_WINDOWEVENT into one event type per reason - RESIZED covers SDL2's SIZE_CHANGED
	const bool isWindowDrag{event.type == SDL_EVENT_WINDOW_MOVED || event.type == SDL_EVENT_WINDOW_RESIZED};
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

void UserInput::SwapControllers(const TabReleasedEvent& event)
{
	bool& isSwapped{event.isSecondPair ? _isSecondPairSwapped : _areControllersSwapped};
	isSwapped = !isSwapped;
	Log::Info("controllers swap state: " + std::to_string(_areControllersSwapped) + '/'
			  + std::to_string(_isSecondPairSwapped));// left while visual label is absent
}

PlayerSlot UserInput::ControllerSlotDefiner(const SDL_JoystickID instanceId) const
{
	const auto isSameId = [instanceId](const std::shared_ptr<SDL_Gamepad>& controller)
	{
		return IsSameController(controller, instanceId);
	};

	//NOTE: an unknown pad falls in with the first - a seat beats none
	const auto it{std::ranges::find_if(_slotsForController, isSameId)};
	const std::size_t index{it == _slotsForController.end()
							? 0u
							: static_cast<std::size_t>(std::distance(_slotsForController.begin(), it))};

	return DeviceSlot(index);
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

		const SDL_Point mouse{ToLogical(event.button.x, event.button.y)};
		if (_isTyping)
		{
			ClickPanelRow(mouse);

			return;
		}

		if (_isMenuShown && SDL_PointInRect(&mouse, &_allTilesRect))
		{
			_isMenuPressHeld = true;
			_events->EmitEvent(EnterEvent{.isPressed = true});
		}

		return;
	}

	if (event.type == SDL_EVENT_MOUSE_BUTTON_UP && event.button.button == SDL_BUTTON_LEFT)
	{
		_mouseButtons.MouseLeftButton = false;
		if (_isMenuPressHeld)
		{
			_isMenuPressHeld = false;
			_events->EmitEvent(EnterEvent{.isPressed = false});
		}

		return;
	}

	if (event.type == SDL_EVENT_MOUSE_MOTION)
	{
		const SDL_Point mouse{ToLogical(event.motion.x, event.motion.y)};
		if (_isTyping)
		{
			HoverPanelRow(mouse);

			return;
		}

		if (_isMenuShown
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

std::optional<std::size_t> UserInput::PanelRowAt(const SDL_Point& mouse) const
{
	const auto isHit = [&mouse](const SDL_Rect& row) { return SDL_PointInRect(&mouse, &row); };
	const auto row{std::ranges::find_if(_panelRows, isHit)};
	if (row == _panelRows.end())
	{
		return std::nullopt;
	}

	return static_cast<std::size_t>(std::distance(_panelRows.begin(), row));
}

void UserInput::ClickPanelRow(const SDL_Point& mouse) const
{
	const std::optional<std::size_t> row{PanelRowAt(mouse)};
	if (!row)
	{
		return;
	}

	const int across{(mouse.x - _panelRows[*row].x) / std::max(_panelSymbolWidth, 1)};
	_events->EmitEvent(PanelRowClickedEvent{.row = *row, .symbol = static_cast<std::size_t>(across)});
}

void UserInput::HoverPanelRow(const SDL_Point& mouse)
{
	const std::optional<std::size_t> row{PanelRowAt(mouse)};
	if (row == _hoveredPanelRow)
	{
		return;
	}

	_hoveredPanelRow = row;
	if (row)
	{
		_events->EmitEvent(PanelRowHoveredEvent{.row = *row});
	}
}

void UserInput::KeyboardKeyPressRelease(const SDL_Event& event, const bool& isPressed) const
{
	const PlayerSlot keyboardLeftSideSlot{DeviceSlot(0u)};
	const PlayerSlot keyboardRightSideSlot{DeviceSlot(1u)};

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
		case SDLK_TAB:
			if (isPressed == false)
			{
				_events->EmitEvent(TabReleasedEvent{.isSecondPair = (event.key.mod & SDL_KMOD_SHIFT) != 0});
			}
			break;
		case SDLK_RETURN:
			_events->EmitEvent(EnterEvent{.isPressed = isPressed});
			break;

		default:
			break;
	}
}

void UserInput::KeyboardEvents(const SDL_Event& event)
{
	if (_isTyping)
	{
		TypingEvents(event);

		return;
	}

	if (event.type == SDL_EVENT_KEY_DOWN)
	{
		KeyboardKeyPressRelease(event, true);
	}
	else if (event.type == SDL_EVENT_KEY_UP)
	{
		KeyboardKeyPressRelease(event, false);
	}
}

//NOTE: no key steers here - A and D are IPv6 hex; Enter does not repeat, or it confirms the screen it opened
void UserInput::TypingEvents(const SDL_Event& event)
{
	if (event.type == SDL_EVENT_TEXT_INPUT)
	{
		if (!std::exchange(_isKeyTyped, false))
		{
			_events->EmitEvent(TextTypedEvent{.text = event.text.text});
		}

		return;
	}

	if (event.type == SDL_EVENT_KEY_UP)
	{
		if (event.key.key == SDLK_RETURN)
		{
			_events->EmitEvent(EnterEvent{.isPressed = false});
		}

		return;
	}

	if (event.type != SDL_EVENT_KEY_DOWN)
	{
		return;
	}

	const std::optional<char> symbol{AddressSymbol(event.key)};
	_isKeyTyped = symbol.has_value();
	if (symbol)
	{
		_events->EmitEvent(TextTypedEvent{.text = std::string(1, *symbol)});

		return;
	}

	const bool isWordJump{(event.key.mod & SDL_KMOD_CTRL) != 0};
	switch (event.key.key)
	{
		case SDLK_RETURN:
			if (!event.key.repeat)
			{
				_events->EmitEvent(EnterEvent{.isPressed = true});
			}
			break;
		case SDLK_BACKSPACE:
			_events->EmitEvent(TextKeyEvent{.key = TextKey::Erase});
			break;
		//NOTE: the pad's dot without Num Lock is Delete too
		case SDLK_DELETE:
		case SDLK_KP_PERIOD:
			_events->EmitEvent(TextKeyEvent{.key = TextKey::EraseRight});
			break;
		//NOTE: a space skips a symbol rather than typing one
		case SDLK_SPACE:
			_events->EmitEvent(TextKeyEvent{.key = TextKey::CaretRight});
			break;
		case SDLK_UP:
			_events->EmitEvent(TextKeyEvent{.key = TextKey::PreviousField});
			break;
		case SDLK_DOWN:
			_events->EmitEvent(TextKeyEvent{.key = TextKey::NextField});
			break;
		case SDLK_LEFT:
			_events->EmitEvent(TextKeyEvent{.key = isWordJump ? TextKey::WordLeft : TextKey::CaretLeft});
			break;
		case SDLK_RIGHT:
			_events->EmitEvent(TextKeyEvent{.key = isWordJump ? TextKey::WordRight : TextKey::CaretRight});
			break;
		case SDLK_TAB:
			_events->EmitEvent(TextKeyEvent{.key = (event.key.mod & SDL_KMOD_SHIFT) != 0 ? TextKey::WordLeft
																						: TextKey::WordRight});
			break;
		case SDLK_V:
			if ((event.key.mod & SDL_KMOD_CTRL) != 0)
			{
				//NOTE: SDL types nothing for Ctrl+V; a failed read is an empty string, never null
				char* const clipboard{SDL_GetClipboardText()};
				_events->EmitEvent(TextPastedEvent{.text = clipboard});
				SDL_free(clipboard);
			}
			break;

		default:
			break;
	}
}

void UserInput::GamepadKeyPressRelease(const SDL_Event& event, const bool& isPressed)
{
	if (ConnectedJoystickCount() > 0)
	{
		const PlayerSlot controllerSlot{ControllerSlotDefiner(event.gbutton.which)};

		switch (event.gbutton.button)
		{
			case SDL_GAMEPAD_BUTTON_SOUTH:
				_events->EmitEvent(Key(LocalInput(controllerSlot)), FireEvent{.isPressed = isPressed});
				break;
			//NOTE: B, X and Y go out as GamepadButtonEvent, which nothing consumes yet - Y also swaps the seats
			case SDL_GAMEPAD_BUTTON_EAST:
				_events->EmitEvent(GamepadButtonEvent{.controllerSlot = controllerSlot,
													  .button = GamepadButton::B,
													  .isPressed = isPressed});
				break;
			case SDL_GAMEPAD_BUTTON_WEST:
				_events->EmitEvent(GamepadButtonEvent{.controllerSlot = controllerSlot,
													  .button = GamepadButton::X,
													  .isPressed = isPressed});
				break;
			case SDL_GAMEPAD_BUTTON_NORTH:
				_events->EmitEvent(GamepadButtonEvent{.controllerSlot = controllerSlot,
													  .button = GamepadButton::Y,
													  .isPressed = isPressed});
				if (isPressed == false)
				{
					_events->EmitEvent(TabReleasedEvent{});
				}
				break;
			case SDL_GAMEPAD_BUTTON_DPAD_UP:
				SteerByDpad(event.gbutton.which, Direction::UP, isPressed);
				break;
			case SDL_GAMEPAD_BUTTON_DPAD_DOWN:
				SteerByDpad(event.gbutton.which, Direction::DOWN, isPressed);
				break;
			case SDL_GAMEPAD_BUTTON_DPAD_LEFT:
				SteerByDpad(event.gbutton.which, Direction::LEFT, isPressed);
				break;
			case SDL_GAMEPAD_BUTTON_DPAD_RIGHT:
				SteerByDpad(event.gbutton.which, Direction::RIGHT, isPressed);
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
		case SDL_EVENT_GAMEPAD_AXIS_MOTION:
		{
			SteerByStick(event.gaxis);
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
			const SDL_JoystickID instanceId{event.gdevice.which};
			Log::Info("controller removed (instance " + std::to_string(instanceId) + ')');
			ReleaseGamepad(instanceId);
			break;
		}

		default:
			break;
	}
}

void UserInput::SteerByDpad(const SDL_JoystickID instanceId, const Direction dir, const bool isPressed)
{
	GamepadDirection& direction{DirectionOf(instanceId)};
	const std::optional<Direction> before{direction.Held()};
	if (isPressed)
	{
		direction.PressDpad(dir);
	}
	else
	{
		direction.ReleaseDpad(dir);
	}

	EmitHeldChange(instanceId, before, direction.Held());
}

void UserInput::SteerByStick(const SDL_GamepadAxisEvent& event)
{
	if (event.axis != SDL_GAMEPAD_AXIS_LEFTX && event.axis != SDL_GAMEPAD_AXIS_LEFTY)
	{
		return;
	}

	GamepadDirection& direction{DirectionOf(event.which)};
	const std::optional<Direction> before{direction.Held()};
	if (event.axis == SDL_GAMEPAD_AXIS_LEFTX)
	{
		direction.MoveStickX(event.value);
	}
	else
	{
		direction.MoveStickY(event.value);
	}

	EmitHeldChange(event.which, before, direction.Held());
}

//NOTE: the held direction goes first and the seat second - a pad pulled out mid-turn would otherwise
//leave the tank driving with nothing to stop it
void UserInput::ReleaseGamepad(const SDL_JoystickID instanceId)
{
	if (const auto it{_gamepadDirections.find(instanceId)}; it != _gamepadDirections.end())
	{
		EmitHeldChange(instanceId, it->second.Held(), std::nullopt);
		_gamepadDirections.erase(it);
	}

	const auto isSameId = [instanceId](const std::shared_ptr<SDL_Gamepad>& controller)
	{
		return IsSameController(controller, instanceId);
	};

	if (const auto it{std::ranges::find_if(_slotsForController, isSameId)}; it != _slotsForController.end())
	{
		it->reset();
	}
}

GamepadDirection& UserInput::DirectionOf(const SDL_JoystickID instanceId)
{
	return _gamepadDirections.try_emplace(instanceId, _gamepadDeadZone).first->second;
}

//NOTE: the tank reads held flags, so only a change is sent - the old side let go before the new one pressed
void UserInput::EmitHeldChange(const SDL_JoystickID instanceId, const std::optional<Direction> before,
							   const std::optional<Direction> after) const
{
	if (before == after)
	{
		return;
	}

	const InputChannel channel{LocalInput(ControllerSlotDefiner(instanceId))};
	if (before)
	{
		EmitMove(channel, *before, false);
	}

	if (after)
	{
		EmitMove(channel, *after, true);
	}
}

void UserInput::EmitMove(const InputChannel channel, const Direction dir, const bool isPressed) const
{
	switch (dir)
	{
		case Direction::UP:
			_events->EmitEvent(Key(channel), MoveUpEvent{.isPressed = isPressed});
			break;
		case Direction::LEFT:
			_events->EmitEvent(Key(channel), MoveLeftEvent{.isPressed = isPressed});
			break;
		case Direction::DOWN:
			_events->EmitEvent(Key(channel), MoveDownEvent{.isPressed = isPressed});
			break;
		case Direction::RIGHT:
			_events->EmitEvent(Key(channel), MoveRightEvent{.isPressed = isPressed});
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
		//NOTE: Esc closes a text field before the game; a held one does not do both
		const bool isEscape{event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_ESCAPE && !event.key.repeat};
		if (isEscape && _isTyping)
		{
			_events->EmitEvent(TextInputCancelledEvent{});
		}
		else if (event.type == SDL_EVENT_QUIT || isEscape)
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

		//NOTE: unlike the reset above, this one does not come back - nothing to rebuild, so say why and leave
		if (event.type == SDL_EVENT_RENDER_DEVICE_LOST)
		{
			Log::Error("render device lost and not recoverable: " + std::string{SDL_GetError()});
			_isShutdown = true;
		}

		WindowDragEvents(event);
		MouseEvents(event);
		KeyboardEvents(event);
		GamepadEvents(event);
	}

	OnWindowDragStop();
}

bool UserInput::IsShutdown() const noexcept { return _isShutdown; }

bool UserInput::IsPause() const noexcept { return _isPause; }

void UserInput::ConnectController(const std::shared_ptr<SDL_Gamepad>& newController)
{
	const auto isEmpty = [](const std::shared_ptr<SDL_Gamepad>& controller) { return controller == nullptr; };

	if (const auto it{std::ranges::find_if(_slotsForController, isEmpty)};
		it != _slotsForController.end())
	{
		*it = newController;
	}
	else
	{
		_slotsForController.push_back(newController);
	}
}

void UserInput::InitControllers()
{
	int joystickCount{};
	SDL_JoystickID* joysticks{SDL_GetJoysticks(&joystickCount)};
	if (joysticks == nullptr)
	{
		Log::Error(std::string{"SDL_GetJoysticks Error: "} + SDL_GetError());

		return;
	}

	Log::Detail(std::to_string(joystickCount) + " gamepad(s) connected");

	//NOTE: one pad per seat
	constexpr auto kMaxControllers{static_cast<int>(kSeatCount)};
	for (int i = 0; i < joystickCount && i < kMaxControllers; ++i)
	{
		if (SDL_Gamepad* gamepad{SDL_OpenGamepad(joysticks[i])};
			gamepad != nullptr)
		{
			ConnectController({gamepad, SDL_CloseGamepad});
			Log::Info("opened controller " + std::to_string(i + 1) + ": " + SDL_GetGamepadName(gamepad));
		}
	}

	SDL_free(joysticks);
}


