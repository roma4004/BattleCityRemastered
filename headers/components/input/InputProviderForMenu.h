#pragma once

#include "components/EventSystem.h"
#include <memory>
#include <vector>

class GameConfig;
class EventSystem;
struct MenuReleasedEvent;
struct PauseReleasedEvent;
struct SetPauseEvent;
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
	std::vector<EventSubscription> _menuNavSubs{};
	const GameConfig& _gameConfig;
	MenuKeys _keys{};

	void OnMenuReleased(const MenuReleasedEvent&);
	void OnPauseReleased(const PauseReleasedEvent&);
	void OnSetPause(const SetPauseEvent& event);
	void OnGameReset(const GameResetEvent&);
	void OnPreTickUpdate(const PreTickUpdateEvent&);
	void OnShowMenu(const ShowMenuEvent& event);
	void OnMenuShowed(const MenuShowedEvent& event);

	void OnMenuNavUp(const MoveUpEvent& event);
	void OnMenuNavDown(const MoveDownEvent& event);
	void OnMenuNavEnter(const EnterEvent& event);
	void OnMenuNavFire(const FireEvent& event);

	void Subscribe();

public:
	InputProviderForMenu(const std::shared_ptr<EventSystem>& events, const GameConfig& gameConfig);

	~InputProviderForMenu() = default;

	void EnableMenuInput();
	void DisableMenuInput();
	void ToggleMenuInputSubscription();
	void Reset();
	void MenuUpdate();
	void TogglePause();
	bool GetPause() const;
	void SetPause(bool value);

	void ToggleUp();
	void ToggleDown();

	[[nodiscard]] MenuKeys GetKeysStats() const noexcept { return _keys; }
};
