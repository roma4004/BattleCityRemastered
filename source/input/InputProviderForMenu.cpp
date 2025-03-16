#include "../../headers/input/InputProviderForMenu.h"
#include "../../headers/EventSystem.h"

InputProviderForMenu::InputProviderForMenu(std::string name, std::shared_ptr<EventSystem> events)
	: _name{std::move(name)}, _events{std::move(events)}
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
	_events->AddListener<const GameMode>("GameModeChangedTo", _name, [this](const GameMode newGameMode)
	{
		this->_gameMode = newGameMode;

		if (_gameMode == PlayAsClient)
		{
			SubscribeAsClient();
		}
		else
		{
			UnsubscribeAsClient();
		}
	});

	if (_gameMode == PlayAsClient)
	{
		SubscribeAsClient();
	}
	else
	{
		UnsubscribeAsClient();
	}
}

void InputProviderForMenu::SubscribeAsClient()
{
	_events->AddListener("ClientReceive_Pause_Released", _name, [this]()//TODO: subscribe only in client
	{
		_keys.pause = !_keys.pause;

		_events->EmitEvent<const bool>("Pause_Status", _keys.pause);
	});
}

void InputProviderForMenu::Unsubscribe() const
{
	_events->RemoveListener("Menu_Released", _name);
	_events->RemoveListener("Pause_Released", _name);\
	_events->RemoveListener<const GameMode>("GameModeChangedTo", _name);
}

void InputProviderForMenu::UnsubscribeAsClient() const
{
	_events->RemoveListener("ClientReceive_Pause_Released", _name);
}

void InputProviderForMenu::TogglePause()
{
	_keys.pause = !_keys.pause;

	_events->EmitEvent<const bool>("Pause_Status", _keys.pause);
}

void InputProviderForMenu::ToggleMenuInputSubscription()
{
	_keys.menuShow = !_keys.menuShow;
	if (_keys.menuShow)
	{
		_events->AddListener("ArrowUp_Released", _name, [&btn = _keys]() { btn.up = true; });
		_events->AddListener("ArrowDown_Released", _name, [&btn = _keys]() { btn.down = true; });
		_events->AddListener("Enter_Pressed", _name, [&btn = _keys]() { btn.reset = true; });
		_events->AddListener("Enter_Released", _name, [&btn = _keys]() { btn.reset = false; });
	}
	else
	{
		_events->RemoveListener("ArrowUp_Released", _name);
		_events->RemoveListener("ArrowDown_Released", _name);
		_events->RemoveListener("Enter_Pressed", _name);
		_events->RemoveListener("Enter_Released", _name);
	}

	_keys.reset = false;
}

void InputProviderForMenu::ToggleUp()
{
	_keys.up = false;
}

void InputProviderForMenu::ToggleDown()
{
	_keys.down = false;
}
