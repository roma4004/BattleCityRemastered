#include "components/input/InputProviderForMenu.h"
#include "components/EventSystem.h"
#include "components/events/CoreLifecycleEvents.h"
#include "components/events/GameModeEvents.h"
#include "components/events/InputEvents.h"
#include "components/events/RenderUIEvents.h"
#include "components/events/TimingEvents.h"
#include "enums/GameMode.h"
#include "network/Client.h"

InputProviderForMenu::InputProviderForMenu(const std::shared_ptr<EventSystem>& events)
	: _events{events}
{
	Subscribe();
}

void InputProviderForMenu::Subscribe()
{
	_subs.push_back(_events->AddListener(this, &InputProviderForMenu::OnMenuReleased));
	_subs.push_back(_events->AddListener(this, &InputProviderForMenu::OnPauseReleased));
	_subs.push_back(_events->AddListener(this, &InputProviderForMenu::OnGameModeChangedTo));
	_subs.push_back(_events->AddListener(this, &InputProviderForMenu::OnGameReset));

	_subs.push_back(_events->AddListener(this, &InputProviderForMenu::OnPreTickUpdate));
	_subs.push_back(_events->AddListener(this, &InputProviderForMenu::OnShowMenu));

	_subs.push_back(_events->AddListener(this, &InputProviderForMenu::OnMenuShowed));
}

void InputProviderForMenu::OnMenuReleased(const MenuReleasedEvent&) { ToggleMenuInputSubscription(); }
void InputProviderForMenu::OnPauseReleased(const PauseReleasedEvent&) { TogglePause(); }
void InputProviderForMenu::OnGameModeChangedTo(const GameModeChangedToEvent& event) { _gameMode = event.mode; }
void InputProviderForMenu::OnGameReset(const GameResetEvent&) { Reset(); }
void InputProviderForMenu::OnPreTickUpdate(const PreTickUpdateEvent&) { MenuUpdate(); }

void InputProviderForMenu::OnShowMenu(const ShowMenuEvent& event)
{
	const bool isDisplayed = event.show;
	if ((isDisplayed && !_keys.menuShow)
		|| (!isDisplayed && _keys.menuShow))
	{
		ToggleMenuInputSubscription();
	}
}

void InputProviderForMenu::OnMenuShowed(const MenuShowedEvent& event)
{
	if (_gameMode == GameMode::Demo
		|| _gameMode == GameMode::PlayAsHost)
	{
		return;
	}

	const bool isDisplayed = event.isShown;
	if (isDisplayed && !_keys.pause
		|| !isDisplayed && _keys.pause)
	{
		TogglePause();
	}
}

// NOTE: registered into _menuNavSubs (not _subs) so DisableMenuInput can drop exactly this
// toggle-able subset by clearing that vector, without disturbing the always-on listeners
// Subscribe() registered into _subs for the lifetime of this object.
void InputProviderForMenu::EnableMenuInput()
{
	_menuNavSubs.push_back(_events->AddListener(Key(std::string{"P1"}), this, &InputProviderForMenu::OnMenuNavUp));
	_menuNavSubs.push_back(_events->AddListener(Key(std::string{"P1"}), this, &InputProviderForMenu::OnMenuNavDown));
	_menuNavSubs.push_back(_events->AddListener(Key(std::string{"P2"}), this, &InputProviderForMenu::OnMenuNavUp));
	_menuNavSubs.push_back(_events->AddListener(Key(std::string{"P2"}), this, &InputProviderForMenu::OnMenuNavDown));
	_menuNavSubs.push_back(_events->AddListener(this, &InputProviderForMenu::OnMenuNavEnter));
	_menuNavSubs.push_back(_events->AddListener(Key(std::string{"P1"}), this, &InputProviderForMenu::OnMenuNavFire));
	_menuNavSubs.push_back(_events->AddListener(Key(std::string{"P2"}), this, &InputProviderForMenu::OnMenuNavFire));
}

void InputProviderForMenu::OnMenuNavUp(const MoveUpEvent& event) { _keys.up = event.isPressed; }
void InputProviderForMenu::OnMenuNavDown(const MoveDownEvent& event) { _keys.down = event.isPressed; }
void InputProviderForMenu::OnMenuNavEnter(const EnterEvent& event) { _keys.reset = event.isPressed; }
void InputProviderForMenu::OnMenuNavFire(const FireEvent& event) { _keys.reset = event.isPressed; }

void InputProviderForMenu::DisableMenuInput() { _menuNavSubs.clear(); }

void InputProviderForMenu::ToggleMenuInputSubscription()
{
	_keys.menuShow = !_keys.menuShow;
	// if (_gameMode != GameMode::PlayAsHost && _gameMode != GameMode::PlayAsClient)
	// {
	// SwitchPause(_keys.menuShow);
	// }

	if (_keys.menuShow)
	{
		EnableMenuInput();
	}
	else
	{
		DisableMenuInput();
	}

	_events->EmitEvent(MenuShowedEvent{.isShown = _keys.menuShow});

	_keys.reset = false;
}

void InputProviderForMenu::ToggleUp()
{
	_keys.up = false;
	_events->EmitEvent(PreviousGameModeEvent{});
}

void InputProviderForMenu::ToggleDown()
{
	_keys.down = false;
	_events->EmitEvent(NextGameModeEvent{});
}

void InputProviderForMenu::TogglePause() { SetPause(!GetPause()); }
// void InputProviderForMenu::SwitchPause(const bool switchTo) { SetPause(switchTo); }

[[nodiscard]] bool InputProviderForMenu::GetPause() const { return _keys.pause; }

void InputProviderForMenu::SetPause(bool value)
{
	if (_keys.pause == value)
	{
		return;
	}

	_keys.pause = value;
	_events->EmitEvent(PauseStatusEvent{.isPaused = _keys.pause});

	if (_gameMode != GameMode::PlayAsClient)
	{
		_events->EmitEvent(ServerOutPauseStatusEvent{.isPaused = _keys.pause});
	}
	else
	{
		_events->EmitEvent(ClientOutPauseStatusEvent{.isPaused = _keys.pause});
	}
}

void InputProviderForMenu::Reset() { SetPause(false); }

void InputProviderForMenu::MenuUpdate()
{
	const auto menuKeysStats = GetKeysStats();

	if (menuKeysStats.up)
	{
		ToggleUp();
	}
	else if (menuKeysStats.down)
	{
		ToggleDown();
	}

	if (menuKeysStats.reset)
	{
		_events->EmitEvent(ApplyGameModeEvent{});
		_events->EmitEvent(ShowMenuEvent{.show = false});
	}
}
