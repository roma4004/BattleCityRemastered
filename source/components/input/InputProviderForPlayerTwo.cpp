#include "components/input/InputProviderForPlayerTwo.h"
#include "components/EventSystem.h"
#include "components/events/InputEvents.h"

InputProviderForPlayerTwo::InputProviderForPlayerTwo(const std::shared_ptr<EventSystem>& events)
	: _events{events} {}

void InputProviderForPlayerTwo::Subscribe()
{
	const std::string tag{"P2"};
	_subs.push_back(_events->AddListener(tag, _name, [&btn = _playerKeys](const MoveUpEvent& event) { btn.up = event.isPressed; }));
	_subs.push_back(_events->AddListener(tag, _name, [&btn = _playerKeys](const MoveLeftEvent& event) { btn.left = event.isPressed; }));
	_subs.push_back(_events->AddListener(tag, _name, [&btn = _playerKeys](const MoveDownEvent& event) { btn.down = event.isPressed; }));
	_subs.push_back(_events->AddListener(tag, _name,
						 [&btn = _playerKeys](const MoveRightEvent& event) { btn.right = event.isPressed; }));
	_subs.push_back(_events->AddListener(tag, _name, [&btn = _playerKeys](const FireEvent& event) { btn.shot = event.isPressed; }));
}

void InputProviderForPlayerTwo::Enable()
{
	Subscribe();
}

void InputProviderForPlayerTwo::Disable() const
{
	_subs.clear();
}
