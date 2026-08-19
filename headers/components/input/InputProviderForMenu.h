#pragma once

#include "components/EventSystem.h"
#include <memory>
#include <vector>

enum class GameMode : char8_t;
class GameConfig;
class EventSystem;
struct MenuReleasedEvent;
struct PauseReleasedEvent;
struct GameResetEvent;
struct PreTickUpdateEvent;
struct ShowMenuEvent;
struct MenuShowedEvent;
struct MoveUpEvent;
struct MoveDownEvent;
struct EnterEvent;
struct FireEvent;

struct MenuKeys final
{
	bool up{false};
	bool down{false};
	bool reset{false};
	bool menuShow{false};
	bool pause{false};
};

class InputProviderForMenu final
{
	std::shared_ptr<EventSystem> _events{nullptr};
	std::vector<EventSubscription> _subs{};
	// Toggled at runtime by EnableMenuInput()/DisableMenuInput(), independent of _subs's fixed
	// subscribe-once-at-construction lifetime - clearing this vector auto-unsubscribes just this group.
	std::vector<EventSubscription> _menuNavSubs{};
	GameConfig& _gameConfig;
	MenuKeys _keys{};

	void OnMenuReleased(const MenuReleasedEvent&);
	void OnPauseReleased(const PauseReleasedEvent&);
	void OnGameReset(const GameResetEvent&);
	void OnPreTickUpdate(const PreTickUpdateEvent&);
	void OnShowMenu(const ShowMenuEvent& event);
	void OnMenuShowed(const MenuShowedEvent& event);

	void OnMenuNavUp(const MoveUpEvent& event);
	void OnMenuNavDown(const MoveDownEvent& event);
	void OnMenuNavEnter(const EnterEvent& event);
	void OnMenuNavFire(const FireEvent& event);

public:
	InputProviderForMenu(const std::shared_ptr<EventSystem>& events, GameConfig& gameConfig);

	~InputProviderForMenu() = default;

	void Subscribe();

	void EnableMenuInput();
	void DisableMenuInput();
	void ToggleMenuInputSubscription();
	void Reset();
	void MenuUpdate();
	void TogglePause();
	// void SwitchPause(bool switchTo);
	bool GetPause() const;
	void SetPause(bool value);

	void ToggleUp();
	void ToggleDown();

	[[nodiscard]] MenuKeys GetKeysStats() const noexcept { return _keys; }
};
