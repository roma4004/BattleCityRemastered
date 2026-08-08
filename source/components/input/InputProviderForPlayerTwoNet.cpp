#include "components/input/InputProviderForPlayerTwoNet.h"
#include "components/EventSystem.h"
#include "components/events/InputEvents.h"

InputProviderForPlayerTwoNet::InputProviderForPlayerTwoNet(const std::shared_ptr<EventSystem>& events)
	: _events{events} {}

InputProviderForPlayerTwoNet::~InputProviderForPlayerTwoNet()
{
	Unsubscribe();
}

void InputProviderForPlayerTwoNet::Subscribe()
{
	const std::string tag{"P2"};
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

void InputProviderForPlayerTwoNet::Unsubscribe() const { _events->RemoveAllListeners(_name); }

void InputProviderForPlayerTwoNet::Enable()
{
	Subscribe();
}

void InputProviderForPlayerTwoNet::Disable() const
{
	Unsubscribe();
}
