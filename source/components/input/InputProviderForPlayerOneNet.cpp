#include "components/input/InputProviderForPlayerOneNet.h"
#include "components/EventSystem.h"
#include "components/events/InputEvents.h"

InputProviderForPlayerOneNet::InputProviderForPlayerOneNet(const std::shared_ptr<EventSystem>& events)
	: _events{events} {}

InputProviderForPlayerOneNet::~InputProviderForPlayerOneNet()
{
	Unsubscribe();
}

void InputProviderForPlayerOneNet::Subscribe()
{
	const std::string tag{"P1"};
	_events->AddListener(tag, _name,
						 [&btn = _playerKeys](const ServerReceiveMoveUpEvent& event) { btn.up = event.isPressed; });
	_events->AddListener(tag, _name,
						 [&btn = _playerKeys](const ServerReceiveMoveLeftEvent& event) { btn.left = event.isPressed; });
	_events->AddListener(tag, _name,
						 [&btn = _playerKeys](const ServerReceiveMoveDownEvent& event) { btn.down = event.isPressed; });
	_events->AddListener(tag, _name, [&btn = _playerKeys](const ServerReceiveMoveRightEvent& event)
	{
		btn.right = event.isPressed;
	});
	_events->AddListener(tag, _name,
						 [&btn = _playerKeys](const ServerReceiveFireEvent& event) { btn.shot = event.isPressed; });

	_events->AddListener(_name, [this](const ServerReceivePauseReleasedEvent& /*event*/)
	{
		_events->EmitEvent(PauseReleasedEvent{});
	});
}

void InputProviderForPlayerOneNet::Unsubscribe() const { _events->RemoveAllListeners(_name); }

void InputProviderForPlayerOneNet::Enable()
{
	Subscribe();
}

void InputProviderForPlayerOneNet::Disable() const
{
	Unsubscribe();
}
