#include "components/input/InputProviderForPlayer.h"
#include "components/EventSystem.h"
#include "components/events/InputEvents.h"

InputProviderForPlayer::InputProviderForPlayer(const std::shared_ptr<EventSystem>& events, const InputChannel channel)
	: _events{events}
	, _channel{channel} {}

void InputProviderForPlayer::Subscribe()
{
	_subs.push_back(_events->AddListener(Key(_channel), this, &InputProviderForPlayer::OnMoveUp));
	_subs.push_back(_events->AddListener(Key(_channel), this, &InputProviderForPlayer::OnMoveLeft));
	_subs.push_back(_events->AddListener(Key(_channel), this, &InputProviderForPlayer::OnMoveDown));
	_subs.push_back(_events->AddListener(Key(_channel), this, &InputProviderForPlayer::OnMoveRight));
	_subs.push_back(_events->AddListener(Key(_channel), this, &InputProviderForPlayer::OnFire));
}

void InputProviderForPlayer::OnMoveUp(const MoveUpEvent& event) { _playerKeys.up = event.isPressed; }
void InputProviderForPlayer::OnMoveLeft(const MoveLeftEvent& event) { _playerKeys.left = event.isPressed; }
void InputProviderForPlayer::OnMoveDown(const MoveDownEvent& event) { _playerKeys.down = event.isPressed; }
void InputProviderForPlayer::OnMoveRight(const MoveRightEvent& event) { _playerKeys.right = event.isPressed; }
void InputProviderForPlayer::OnFire(const FireEvent& event) { _playerKeys.shot = event.isPressed; }

void InputProviderForPlayer::Enable()
{
	Subscribe();
}

void InputProviderForPlayer::Disable()
{
	_subs.clear();
}
