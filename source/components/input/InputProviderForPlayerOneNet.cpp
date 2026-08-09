#include "components/input/InputProviderForPlayerOneNet.h"
#include "components/EventSystem.h"
#include "components/events/InputEvents.h"

InputProviderForPlayerOneNet::InputProviderForPlayerOneNet(const std::shared_ptr<EventSystem>& events)
	: _events{events} {}

void InputProviderForPlayerOneNet::Subscribe()
{
	const std::string tag{"P1"};
	_subs.push_back(_events->AddListener(tag, _name, [&btn = _playerKeys](const ServerInMoveUpEvent& event)
	{
		btn.up = event.isPressed;
	}));
	_subs.push_back(_events->AddListener(tag, _name, [&btn = _playerKeys](const ServerInMoveLeftEvent& event)
	{
		btn.left = event.isPressed;
	}));
	_subs.push_back(_events->AddListener(tag, _name, [&btn = _playerKeys](const ServerInMoveDownEvent& event)
	{
		btn.down = event.isPressed;
	}));
	_subs.push_back(_events->AddListener(tag, _name, [&btn = _playerKeys](const ServerInMoveRightEvent& event)
	{
		btn.right = event.isPressed;
	}));
	_subs.push_back(_events->AddListener(tag, _name, [&btn = _playerKeys](const ServerInFireEvent& event)
	{
		btn.shot = event.isPressed;
	}));

	_subs.push_back(_events->AddListener(_name, [this](const ServerInPauseReleasedEvent& /*event*/)
	{
		_events->EmitEvent(PauseReleasedEvent{});
	}));
}

void InputProviderForPlayerOneNet::Enable()
{
	Subscribe();
}

void InputProviderForPlayerOneNet::Disable() const
{
	_subs.clear();
}
