#pragma once

#include "geometry/Point.h"
#include "../components/input/MouseButton.h"
#include "components/EventSystem.h"
#include "components/input/GamepadDirection.h"
#include "components/input/InputProviderForMenu.h"
#include "enums/InputChannel.h"
#include "enums/PlayerSlot.h"
#include <SDL3/SDL_gamepad.h>
#include <SDL3/SDL_rect.h>
#include <chrono>
#include <memory>
#include <optional>
#include <unordered_map>
#include <vector>

enum class Direction : char8_t;
enum class GameMode : char8_t;
union SDL_Event;
struct SDL_GamepadAxisEvent;
struct SDL_Config;
struct ServerScreenShownEvent;
struct PanelRowsPlacedEvent;
struct PauseStatusEvent;
struct TabReleasedEvent;
struct PreTickUpdateEvent;
struct MenuShownEvent;
struct MenuTilesPlacedEvent;
class EventSystem;
class WindowConfig;

class UserInput final
{
	struct SubTile
	{
		SDL_Rect rect;
		GameMode gameMode;
	};

	using milliseconds = std::chrono::milliseconds;

	MouseButtons _mouseButtons{};
	bool _isShutdown{};
	bool _isPause{};
	bool _isPausedByWindowDrag{};
	bool _isWindowDragging{};
	bool _isMenuShown{};
	//NOTE: a release is only the other half of a press that landed on a menu item - loose ones reach
	//whoever else listens for Enter, and the won scoreboard would take a click meant for the window
	bool _isMenuPressHeld{};
	//NOTE: the keys type instead of steering, and Esc closes the field
	bool _isTyping{};
	//NOTE: the last key down typed its symbol itself - the layout's text for that press is dropped
	bool _isKeyTyped{};
	GameMode _selectedGameMode{};
	bool _areControllersSwapped{};
	UPoint _windowSize{};
	std::shared_ptr<EventSystem> _events{nullptr};
	std::vector<EventSubscription> _subs{};
	std::chrono::steady_clock::time_point _lastDragEventTime{};
	milliseconds _dragEndDelay{150};
	std::vector<std::shared_ptr<SDL_Gamepad>> _slotsForController{};
	std::unordered_map<SDL_JoystickID, GamepadDirection> _gamepadDirections{};
	int _gamepadDeadZone;
	const SDL_Config& _sdlConfig;
	//NOTE: the rows as the renderer drew them - nothing here works them out, a click just lands on them
	SDL_Rect _allTilesRect{};
	std::vector<SubTile> _menuTiles;
	std::vector<SDL_Rect> _panelRows{};

	void MouseEvents(const SDL_Event& event);
	void ClickPanelRow(const SDL_Point& mouse) const;
	[[nodiscard]] SDL_Point ToLogical(float windowX, float windowY) const;
	void KeyboardKeyPressRelease(const SDL_Event& event, const bool& isPressed) const;
	void KeyboardEvents(const SDL_Event& event);
	void TypingEvents(const SDL_Event& event);
	void GamepadKeyPressRelease(const SDL_Event& event, const bool& isPressed);
	void GamepadEvents(const SDL_Event& event);
	void SteerByDpad(SDL_JoystickID instanceId, Direction dir, bool isPressed);
	void SteerByStick(const SDL_GamepadAxisEvent& event);
	void ReleaseGamepad(SDL_JoystickID instanceId);
	[[nodiscard]] GamepadDirection& DirectionOf(SDL_JoystickID instanceId);
	void EmitHeldChange(SDL_JoystickID instanceId, std::optional<Direction> before,
						std::optional<Direction> after) const;
	void EmitMove(InputChannel channel, Direction dir, bool isPressed) const;
	void OnWindowDragStop();
	void WindowDragEvents(const SDL_Event& event);
	void OnWindowResized(UPoint newSize);

	void Subscribe();
	void OnPauseStatus(const PauseStatusEvent& event);
	void SwapControllers(const TabReleasedEvent&);
	void OnPreTickUpdate(const PreTickUpdateEvent&);
	void OnMenuShown(const MenuShownEvent& event);
	void OnServerScreenShown(const ServerScreenShownEvent& event);
	void OnPanelRowsPlaced(const PanelRowsPlacedEvent& event);

	void InitControllers();
	void ConnectController(const std::shared_ptr<SDL_Gamepad>& newController);
	[[nodiscard]] PlayerSlot ControllerSlotDefiner(SDL_JoystickID instanceId) const;
	void OnMenuTilesPlaced(const MenuTilesPlacedEvent& event);

public:
	UserInput(const std::shared_ptr<EventSystem>& events, const WindowConfig& windowConfig,
			  const SDL_Config& sdlConfig, int gamepadDeadZone);
	~UserInput();

	void Update();

	[[nodiscard]] bool IsShutdown() const noexcept;
	[[nodiscard]] bool IsPause() const noexcept;
};
