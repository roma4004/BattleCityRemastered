#include "components/input/InputProviderForPlayerTwoNet.h"
#include "components/EventSystem.h"

InputProviderForPlayerTwoNet::InputProviderForPlayerTwoNet(const std::shared_ptr<EventSystem>& events)
	: _events{events} {}

InputProviderForPlayerTwoNet::~InputProviderForPlayerTwoNet()
{
	Unsubscribe();
}

void InputProviderForPlayerTwoNet::Subscribe()
{
	const std::string tag{"P2"};
	_events->AddListener("ServerReceive_Move_Up", tag, _name,
						 [&btn = _playerKeys](const bool isPressed) { btn.up = isPressed; });
	_events->AddListener("ServerReceive_Move_Left", tag, _name,
						 [&btn = _playerKeys](const bool isPressed) { btn.left = isPressed; });
	_events->AddListener("ServerReceive_Move_Down", tag, _name,
						 [&btn = _playerKeys](const bool isPressed) { btn.down = isPressed; });
	_events->AddListener("ServerReceive_Move_Right", tag, _name, [&btn = _playerKeys](const bool isPressed)
	{
		btn.right = isPressed;
	});
	_events->AddListener("ServerReceive_Fire", tag, _name,
						 [&btn = _playerKeys](const bool isPressed) { btn.shot = isPressed; });

	_events->AddListener("ServerReceive_Pause_Released", _name, [this](const bool /*isPaused*/)
	{
		_events->EmitEvent("Pause_Released");
	});
}

void InputProviderForPlayerTwoNet::Unsubscribe() const { _events->RemoveAllListeners(_name); }

void InputProviderForPlayerTwoNet::Enable()
{
	Subscribe();
}

void InputProviderForPlayerTwoNet::Disable() const
{
	Unsubscribe();
}
