#include "components/input/InputProviderForPlayerOne.h"
#include "components/EventSystem.h"
#include "components/events/InputEvents.h"

InputProviderForPlayerOne::InputProviderForPlayerOne(const std::shared_ptr<EventSystem>& events)
	: _events{events} {}

InputProviderForPlayerOne::~InputProviderForPlayerOne()
{
	Unsubscribe();
}

void InputProviderForPlayerOne::Subscribe()
{
	const std::string tag{"P1"};
	_events->AddListener(tag, _name, [&btn = _playerKeys](const MoveUpEvent& event) { btn.up = event.isPressed; });
	_events->AddListener(tag, _name, [&btn = _playerKeys](const MoveLeftEvent& event) { btn.left = event.isPressed; });
	_events->AddListener(tag, _name, [&btn = _playerKeys](const MoveDownEvent& event) { btn.down = event.isPressed; });
	_events->AddListener(tag, _name,
						 [&btn = _playerKeys](const MoveRightEvent& event) { btn.right = event.isPressed; });
	_events->AddListener(tag, _name, [&btn = _playerKeys](const FireEvent& event) { btn.shot = event.isPressed; });
}

void InputProviderForPlayerOne::Unsubscribe() const { _events->RemoveAllListeners(_name); }

void InputProviderForPlayerOne::Enable()
{
	Subscribe();
}

void InputProviderForPlayerOne::Disable() const
{
	Unsubscribe();
}
