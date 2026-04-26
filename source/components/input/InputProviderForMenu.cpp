#include "components/input/InputProviderForMenu.h"
#include "components/EventSystem.h"
#include "enums/GameMode.h"
#include "network/Client.h"

InputProviderForMenu::InputProviderForMenu(const std::shared_ptr<EventSystem>& events)
	: _events{events}
{
	Subscribe();
}

InputProviderForMenu::~InputProviderForMenu()
{
	Unsubscribe();
}

void InputProviderForMenu::Subscribe()
{
	_events->AddListener("Menu_Released", _name, [this]() { this->ToggleMenuInputSubscription(); });
	_events->AddListener("ScoreBoardShowed", _name, [this](const bool isDisplayed)
	{
		this->OnScoreBoardShowed(isDisplayed);
	});
	_events->AddListener("Pause_Released", _name, [this]() { this->TogglePause(); });
	_events->AddListener("GameModeChangedTo", _name, [this](const GameMode newGameMode)
	{
		this->_gameMode = newGameMode;
		this->ToggleMenuInputSubscription();
	});
	_events->AddListener("Reset", _name, [this]() { this->Reset(); });

	_events->AddListener("PreTickUpdate", _name, [this](const double /*deltaTime*/) { this->MenuUpdate(); });
}

void InputProviderForMenu::Unsubscribe() const
{
	_events->RemoveListener("Menu_Released", _name);
	_events->RemoveListener("ScoreBoardShowed", _name);
	_events->RemoveListener("Pause_Released", _name);
	_events->RemoveListener("GameModeChangedTo", _name);
	_events->RemoveListener("Reset", _name);
	_events->RemoveListener("PreTickUpdate", _name);

	if (_keys.menuShow)
	{
		DisableMenuInput();
	}
}

void InputProviderForMenu::EnableMenuInput()
{
	_events->AddListener("P1_Move_Up", _name, [&btn = _keys](const bool isPressed) { btn.up = isPressed; });
	_events->AddListener("P1_Move_Down", _name, [&btn = _keys](const bool isPressed) { btn.down = isPressed; });
	_events->AddListener("P2_Move_Up", _name, [&btn = _keys](const bool isPressed) { btn.up = isPressed; });
	_events->AddListener("P2_Move_Down", _name, [&btn = _keys](const bool isPressed) { btn.down = isPressed; });
	_events->AddListener("Enter", _name, [&btn = _keys](const bool isPressed) { btn.reset = isPressed; });
	_events->AddListener("P1_Fire", _name, [&btn = _keys](const bool isPressed) { btn.reset = isPressed; });
	_events->AddListener("P2_Fire", _name, [&btn = _keys](const bool isPressed) { btn.reset = isPressed; });

}

void InputProviderForMenu::DisableMenuInput() const
{
	_events->RemoveListener("P1_Move_Up", _name);
	_events->RemoveListener("P1_Move_Down", _name);
	_events->RemoveListener("P2_Move_Up", _name);
	_events->RemoveListener("P2_Move_Down", _name);
	_events->RemoveListener("Enter", _name);
	_events->RemoveListener("P1_Fire", _name);
	_events->RemoveListener("P2_Fire", _name);
}

void InputProviderForMenu::OnScoreBoardShowed(const bool isDisplayed)
{
	if (isDisplayed)
	{
		ToggleMenuInputSubscription();
	}
	else
	{
		_isScoreBoardDisplayed = false;
	}
}

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

	_events->EmitEvent("ShowMenu", _keys.menuShow);

	_keys.reset = false;
}

void InputProviderForMenu::ToggleUp()
{
	_keys.up = false;
	_events->EmitEvent("PreviousGameMode");
}

void InputProviderForMenu::ToggleDown()
{
	_keys.down = false;
	_events->EmitEvent("NextGameMode");
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
	_events->EmitEvent("Pause_Status", _keys.pause);

	if (_gameMode != GameMode::PlayAsClient)
	{
		_events->EmitEvent("ServerSend_Pause_Status", _keys.pause);
	}
	else
	{
		_events->EmitEvent("ClientSend_Pause_Status", _keys.pause);
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
		_events->EmitEvent("ResetBattlefield");
		ToggleMenuInputSubscription();
	}
}
