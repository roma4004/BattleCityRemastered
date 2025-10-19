#include "components/input/InputProviderForMenu.h"
#include "application/UserInput.h"
#include "components/EventSystem.h"
#include "enums/GameMode.h"

InputProviderForMenu::InputProviderForMenu(std::shared_ptr<EventSystem> events)
	: _events{std::move(events)}
{
	Subscribe();
}

InputProviderForMenu::~InputProviderForMenu()
{
	Unsubscribe();
}

void InputProviderForMenu::Subscribe()
{
	this->ToggleMenuInputSubscription();

	_events->AddListener("Menu_Released", _name, [this]() { this->ToggleMenuInputSubscription(); });
	_events->AddListener("Pause_Released", _name, [this]() { this->TogglePause(); });
	_events->AddListener("GameModeChangedTo", _name, [this](const GameMode newGameMode)
	{
		this->_gameMode = newGameMode;
		_gameMode == GameMode::PlayAsClient ? SubscribeAsClient() : UnsubscribeAsClient();
	});
}

void InputProviderForMenu::SubscribeAsClient()
{
	_events->AddListener("ClientReceive_Pause_Released", _name, [this]()
	{
		_keys.pause = !_keys.pause;

		_events->EmitEvent("Pause_Status", _keys.pause);
	});
}

void InputProviderForMenu::Unsubscribe() const
{
	_events->RemoveListener("Menu_Released", _name);
	_events->RemoveListener("Pause_Released", _name);
	_events->RemoveListener("GameModeChangedTo", _name);

	if (_gameMode == GameMode::PlayAsClient)
	{
		UnsubscribeAsClient();
	}
}

void InputProviderForMenu::UnsubscribeAsClient() const
{
	_events->RemoveListener("ClientReceive_Pause_Released", _name);
}

void InputProviderForMenu::TogglePause()
{
	_keys.pause = !_keys.pause;

	_events->EmitEvent("Pause_Status", _keys.pause);
}

void InputProviderForMenu::ToggleMenuInputSubscription()
{
	_keys.menuShow = !_keys.menuShow;
	if (_keys.menuShow)
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
	else
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

	_keys.reset = false;
}

void InputProviderForMenu::ToggleUp() { _keys.up = false; }
void InputProviderForMenu::ToggleDown() { _keys.down = false; }
