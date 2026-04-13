#include "components/input/InputProviderForPlayerOneNet.h"
#include "components/EventSystem.h"

InputProviderForPlayerOneNet::InputProviderForPlayerOneNet(const std::shared_ptr<EventSystem>& events)
	: _events{events} {}

InputProviderForPlayerOneNet::~InputProviderForPlayerOneNet()
{
	Unsubscribe();
}

void InputProviderForPlayerOneNet::Subscribe()
{
	_events->AddListener("ServerReceive_P1_Move_Up", _name, [&btn = _playerKeys](const bool isPressed) { btn.up = isPressed; });
	_events->AddListener("ServerReceive_P1_Move_Left", _name, [&btn = _playerKeys](const bool isPressed) { btn.left = isPressed; });
	_events->AddListener("ServerReceive_P1_Move_Down", _name, [&btn = _playerKeys](const bool isPressed) { btn.down = isPressed; });
	_events->AddListener("ServerReceive_P1_Move_Right", _name, [&btn = _playerKeys](const bool isPressed) { btn.right = isPressed; });
	_events->AddListener("ServerReceive_P1_Fire", _name, [&btn = _playerKeys](const bool isPressed) { btn.shot = isPressed; });

	_events->AddListener("ServerReceive_Pause_Released", _name, [this](const bool /*isPaused*/)
	{
		_events->EmitEvent("Pause_Released");
	});
}

void InputProviderForPlayerOneNet::Unsubscribe() const
{
	_events->RemoveListener("ServerReceive_P1_Move_Up", _name);
	_events->RemoveListener("ServerReceive_P1_Move_Left", _name);
	_events->RemoveListener("ServerReceive_P1_Move_Down", _name);
	_events->RemoveListener("ServerReceive_P1_Move_Right", _name);
	_events->RemoveListener("ServerReceive_P1_Fire", _name);

	_events->RemoveListener("ServerReceive_Pause_Released", _name);
}

void InputProviderForPlayerOneNet::Enable()
{
	Subscribe();
}

void InputProviderForPlayerOneNet::Disable() const
{
	Unsubscribe();
}
