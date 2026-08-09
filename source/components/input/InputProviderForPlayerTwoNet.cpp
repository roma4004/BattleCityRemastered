#include "components/input/InputProviderForPlayerTwoNet.h"
#include "components/EventSystem.h"
#include "components/events/InputEvents.h"

InputProviderForPlayerTwoNet::InputProviderForPlayerTwoNet(const std::shared_ptr<EventSystem>& events)
	: _events{events} {}

void InputProviderForPlayerTwoNet::Subscribe()
{
	const std::string tag{"P2"};
	_subs.push_back(_events->AddListener(tag, _name, [&btn = _playerKeys](const ServerReceiveMoveUpEvent& event)
	{
		btn.up = event.isPressed;
	}));
	_subs.push_back(_events->AddListener(tag, _name, [&btn = _playerKeys](const ServerReceiveMoveLeftEvent& event)
	{
		btn.left = event.isPressed;
	}));
	_subs.push_back(_events->AddListener(tag, _name, [&btn = _playerKeys](const ServerReceiveMoveDownEvent& event)
	{
		btn.down = event.isPressed;
	}));
	_subs.push_back(_events->AddListener(tag, _name, [&btn = _playerKeys](const ServerReceiveMoveRightEvent& event)
	{
		btn.right = event.isPressed;
	}));
	_subs.push_back(_events->AddListener(tag, _name, [&btn = _playerKeys](const ServerReceiveFireEvent& event)
	{
		btn.shot = event.isPressed;
	}));

	_subs.push_back(_events->AddListener(_name, [this](const ServerReceivePauseReleasedEvent& /*event*/)
	{
		_events->EmitEvent(PauseReleasedEvent{});
	}));
}

void InputProviderForPlayerTwoNet::Enable()
{
	Subscribe();
}

void InputProviderForPlayerTwoNet::Disable() const
{
	_subs.clear();
}
