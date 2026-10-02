#pragma once

#include "components/EventSystem.h"
#include <memory>
#include <vector>

struct MenuReleasedEvent;
struct PauseReleasedEvent;
struct SetPauseEvent;
struct GameResetEvent;
struct PreTickUpdateEvent;
struct ShowMenuEvent;
struct MenuShownEvent;
struct ServerScreenShownEvent;
struct MoveUpEvent;
struct MoveDownEvent;
struct EnterEvent;
struct FireEvent;
class GameConfig;
class EventSystem;

struct MenuKeys final
{
	bool up{};
	bool down{};
	bool reset{};
	bool menuShow{};
	bool pause{};
};

class InputProviderForMenu final
{
	std::shared_ptr<EventSystem> _events{nullptr};
	std::vector<EventSubscription> _subs{};
	//NOTE: apart from _subs because the menu keys come and go with the menu, and clearing this vector
	//is how they go - the listeners in _subs stay for the life of the object
	std::vector<EventSubscription> _menuNavSubs{};
	const GameConfig& _gameConfig;
	MenuKeys _keys{};
	//NOTE: it stands in for the menu, so hiding the menu for it is no unpause
	bool _isServerScreenShown{};

	void OnMenuReleased(const MenuReleasedEvent&);
	void OnPauseReleased(const PauseReleasedEvent&);
	void OnSetPause(const SetPauseEvent& event);
	void OnGameReset(const GameResetEvent&);
	void OnPreTickUpdate(const PreTickUpdateEvent&);
	void OnShowMenu(const ShowMenuEvent& event);
	void OnMenuShown(const MenuShownEvent& event);
	void OnServerScreenShown(const ServerScreenShownEvent& event);

	void OnMenuNavUp(const MoveUpEvent& event);
	void OnMenuNavDown(const MoveDownEvent& event);
	void OnMenuNavEnter(const EnterEvent& event);
	void OnMenuNavFire(const FireEvent& event);

	void Subscribe();

public:
	InputProviderForMenu(const std::shared_ptr<EventSystem>& events, const GameConfig& gameConfig);

	void EnableMenuInput();
	void DisableMenuInput();
	void ToggleMenuInputSubscription();
	void Reset();
	void MenuUpdate();
	void TogglePause();
	bool GetPause() const noexcept;
	void SetPause(bool value);

	void ToggleUp();
	void ToggleDown();

	[[nodiscard]] MenuKeys GetKeysStats() const noexcept { return _keys; }
};
