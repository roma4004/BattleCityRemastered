#include "components/input/InputProviderForMenu.h"
#include "application/GameConfig.h"
#include "components/EventSystem.h"
#include "components/events/CoreLifecycleEvents.h"
#include "components/events/InputEvents.h"
#include "components/events/RenderUIEvents.h"
#include "components/events/TimingEvents.h"
#include "enums/GameMode.h"
#include "enums/PlayerSlot.h"

InputProviderForMenu::InputProviderForMenu(const std::shared_ptr<EventSystem>& events, const GameConfig& gameConfig)
	: _events{events}
	, _gameConfig{gameConfig}
{
	Subscribe();
}

void InputProviderForMenu::Subscribe()
{
	_subs.push_back(_events->AddListener(this, &InputProviderForMenu::OnMenuReleased));
	_subs.push_back(_events->AddListener(this, &InputProviderForMenu::OnPauseReleased));
	_subs.push_back(_events->AddListener(this, &InputProviderForMenu::OnSetPause));
	_subs.push_back(_events->AddListener(this, &InputProviderForMenu::OnGameReset));
	_subs.push_back(_events->AddListener(this, &InputProviderForMenu::OnGameStateChangedTo));

	_subs.push_back(_events->AddListener(this, &InputProviderForMenu::OnPreTickUpdate));
	_subs.push_back(_events->AddListener(this, &InputProviderForMenu::OnShowMenu));

	_subs.push_back(_events->AddListener(this, &InputProviderForMenu::OnMenuShowed));
}

void InputProviderForMenu::OnMenuReleased(const MenuReleasedEvent&) { ToggleMenuInputSubscription(); }
void InputProviderForMenu::OnPauseReleased(const PauseReleasedEvent&) { TogglePause(); }
void InputProviderForMenu::OnSetPause(const SetPauseEvent& event) { SetPause(event.isPaused); }
void InputProviderForMenu::OnGameReset(const GameResetEvent&) { Reset(); }
void InputProviderForMenu::OnGameStateChangedTo(const GameStateChangedToEvent& event) { _gameState = event.state; }
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

//TODO: change direction without move (one turn before move)
void InputProviderForMenu::OnMenuShowed(const MenuShowedEvent& event)
{
	//NOTE: neither owns the pause here - Demo runs behind an open menu, a lobby is not running at all
	if (_gameConfig.gameMode == GameMode::Demo || _gameState == GameState::Lobby)
	{
		return;
	}

	const bool isDisplayed = event.isShown;
	if (isDisplayed != _keys.pause)//NOTE: menu visibility and the pause flag drifted apart
	{
		TogglePause();
	}
}

// NOTE: registered into _menuNavSubs (not _subs) so DisableMenuInput can drop exactly this
// toggle-able subset by clearing that vector, without disturbing the always-on listeners
// Subscribe() registered into _subs for the lifetime of this object.
void InputProviderForMenu::EnableMenuInput()
{
	_menuNavSubs.push_back(_events->AddListener(this, &InputProviderForMenu::OnMenuNavEnter));

	_menuNavSubs.push_back(_events->AddListener(Key(PlayerSlot::P1), this, &InputProviderForMenu::OnMenuNavUp));
	_menuNavSubs.push_back(_events->AddListener(Key(PlayerSlot::P1), this, &InputProviderForMenu::OnMenuNavDown));
	_menuNavSubs.push_back(_events->AddListener(Key(PlayerSlot::P1), this, &InputProviderForMenu::OnMenuNavFire));

	_menuNavSubs.push_back(_events->AddListener(Key(PlayerSlot::P2), this, &InputProviderForMenu::OnMenuNavUp));
	_menuNavSubs.push_back(_events->AddListener(Key(PlayerSlot::P2), this, &InputProviderForMenu::OnMenuNavDown));
	_menuNavSubs.push_back(_events->AddListener(Key(PlayerSlot::P2), this, &InputProviderForMenu::OnMenuNavFire));
}

void InputProviderForMenu::OnMenuNavUp(const MoveUpEvent& event) { _keys.up = event.isPressed; }
void InputProviderForMenu::OnMenuNavDown(const MoveDownEvent& event) { _keys.down = event.isPressed; }
void InputProviderForMenu::OnMenuNavEnter(const EnterEvent& event) { _keys.reset = event.isPressed; }
void InputProviderForMenu::OnMenuNavFire(const FireEvent& event) { _keys.reset = event.isPressed; }

void InputProviderForMenu::DisableMenuInput() { _menuNavSubs.clear(); }

void InputProviderForMenu::ToggleMenuInputSubscription()
{
	_keys.menuShow = !_keys.menuShow;

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

void InputProviderForMenu::TogglePause()
{
	SetPause(!GetPause());

	if (!_gameConfig.IsHost())
	{
		_events->EmitEvent(PauseRequestedEvent{.isPaused = _keys.pause});
	}
}

[[nodiscard]] bool InputProviderForMenu::GetPause() const { return _keys.pause; }

void InputProviderForMenu::SetPause(bool value)
{
	if (_keys.pause == value)
	{
		return;
	}

	_keys.pause = value;
	_events->EmitEvent(PauseStatusEvent{.isPaused = _keys.pause});
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
	}
}
