#include "components/input/InputProviderForMenu.h"
#include "components/EventSystem.h"
#include "enums/GameMode.h"

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
	ToggleMenuInputSubscription();
	_events->AddListener("Menu_Released", _name, [this]() { this->ToggleMenuInputSubscription(); });
	_events->AddListener("Pause_Released", _name, [this]() { this->TogglePause(); });
	_events->AddListener("GameModeChangedTo", _name, [this](const GameMode newGameMode)
	{
		this->_gameMode = newGameMode;
	});
	_events->AddListener("Reset", _name, [this]() { this->Reset(); });

	_events->AddListener("PreTickUpdate", _name, [this](const double /*deltaTime*/) { this->MenuUpdate(); });
}

void InputProviderForMenu::Unsubscribe() const
{
	_events->RemoveListener("Menu_Released", _name);
	_events->RemoveListener("Pause_Released", _name);
	_events->RemoveListener("GameModeChangedTo", _name);
	_events->RemoveListener("Reset", _name);
	_events->RemoveListener("PreTickUpdate", _name);
}

void InputProviderForMenu::EnableMenuInput()
{
	_events->AddListener("P1_Move_Up_Released", _name, [&btn = _keys]() { btn.up = true; });
	_events->AddListener("P1_Move_Down_Released", _name, [&btn = _keys]() { btn.down = true; });
	_events->AddListener("P2_Move_Up_Released", _name, [&btn = _keys]() { btn.up = true; });
	_events->AddListener("P2_Move_Down_Released", _name, [&btn = _keys]() { btn.down = true; });
	_events->AddListener("Enter_Pressed", _name, [&btn = _keys]() { btn.reset = true; });
	_events->AddListener("Enter_Released", _name, [&btn = _keys]() { btn.reset = false; });
	_events->AddListener("P1_Fire_Pressed", _name, [&btn = _keys]() { btn.reset = true; });
	_events->AddListener("P1_Fire_Released", _name, [&btn = _keys]() { btn.reset = false; });
	_events->AddListener("P2_Fire_Pressed", _name, [&btn = _keys]() { btn.reset = true; });
	_events->AddListener("P2_Fire_Released", _name, [&btn = _keys]() { btn.reset = false; });
}

void InputProviderForMenu::DisableMenuInput() const
{
	_events->RemoveListener("P1_Move_Up_Released", _name);
	_events->RemoveListener("P1_Move_Down_Released", _name);
	_events->RemoveListener("P2_Move_Up_Released", _name);
	_events->RemoveListener("P2_Move_Down_Released", _name);
	_events->RemoveListener("Enter_Pressed", _name);
	_events->RemoveListener("Enter_Released", _name);
	_events->RemoveListener("P1_Fire_Pressed", _name);
	_events->RemoveListener("P1_Fire_Released", _name);
	_events->RemoveListener("P2_Fire_Pressed", _name);
	_events->RemoveListener("P2_Fire_Released", _name);
}

void InputProviderForMenu::ToggleMenuInputSubscription()
{
	_keys.menuShow = !_keys.menuShow;//TODO: add setter
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
void InputProviderForMenu::SwitchPause(const bool switchTo) { SetPause(switchTo); }

[[nodiscard]] bool InputProviderForMenu::GetPause() const { return _keys.pause; }

void InputProviderForMenu::SetPause(bool value)
{
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