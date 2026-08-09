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
	_subs.push_back(_events->AddListener(_name, [this](const MenuReleasedEvent&)
	{
		this->ToggleMenuInputSubscription();
	}));
	_subs.push_back(_events->AddListener(_name, [this](const PauseReleasedEvent&) { this->TogglePause(); }));
	_subs.push_back(_events->AddListener(_name, [this](const GameModeChangedToEvent& event)
	{
		this->_gameMode = event.mode;
	}));
	_subs.push_back(_events->AddListener(_name, [this](const GameResetEvent&) { this->Reset(); }));

	_subs.push_back(_events->AddListener(_name, [this](const PreTickUpdateEvent& /*event*/) { this->MenuUpdate(); }));
	_subs.push_back(_events->AddListener(_name, [this](const ShowMenuEvent& event)
	{
		const bool isDisplayed = event.show;
		if ((isDisplayed && !_keys.menuShow)
			|| (!isDisplayed && _keys.menuShow))
		{
			this->ToggleMenuInputSubscription();
		}
	}));

	_subs.push_back(_events->AddListener(_name, [this](const MenuShowedEvent& event)
	{
		if (this->_gameMode == GameMode::Demo
			|| this->_gameMode == GameMode::PlayAsHost)
		{
			return;
		}

		const bool isDisplayed = event.isShown;
		if (isDisplayed && !_keys.pause
			|| !isDisplayed && _keys.pause)
		{
			this->TogglePause();
		}
	}));
}

// NOTE: registered under _menuNavName (not _name) so DisableMenuInput can drop exactly this
// toggle-able subset via clearing _menuNavSubs, without disturbing the always-on listeners
// Subscribe() registered under _name for the lifetime of this object.
void InputProviderForMenu::EnableMenuInput()
{
	const std::string menuNavName{_name + "_MenuNav"};
	_menuNavSubs.push_back(_events->AddListener(std::string{"P1"}, menuNavName, [&btn = _keys](const MoveUpEvent& event)
	{
		btn.up = event.isPressed;
	}));
	_menuNavSubs.push_back(
			_events->AddListener(std::string{"P1"}, menuNavName, [&btn = _keys](const MoveDownEvent& event)
			{
				btn.down = event.isPressed;
			}));
	_menuNavSubs.push_back(_events->AddListener(std::string{"P2"}, menuNavName, [&btn = _keys](const MoveUpEvent& event)
	{
		btn.up = event.isPressed;
	}));
	_menuNavSubs.push_back(
			_events->AddListener(std::string{"P2"}, menuNavName, [&btn = _keys](const MoveDownEvent& event)
			{
				btn.down = event.isPressed;
			}));
	_menuNavSubs.push_back(_events->AddListener(menuNavName, [&btn = _keys](const EnterEvent& event)
	{
		btn.reset = event.isPressed;
	}));
	_menuNavSubs.push_back(_events->AddListener(std::string{"P1"}, menuNavName, [&btn = _keys](const FireEvent& event)
	{
		btn.reset = event.isPressed;
	}));
	_menuNavSubs.push_back(_events->AddListener(std::string{"P2"}, menuNavName, [&btn = _keys](const FireEvent& event)
	{
		btn.reset = event.isPressed;
	}));
}

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
