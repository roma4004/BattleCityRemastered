#include "components/input/InputProviderForPlayer.h"
#include "components/EventSystem.h"
#include "components/events/InputEvents.h"
#include "enums/Direction.h"

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

//NOTE: same priority the tank spelled out inline - up, then left, then down, then right
std::optional<Direction> InputProviderForPlayer::ChooseDirection(Tank& /*self*/, const double /*deltaTime*/)
{
	if (_playerKeys.up)
	{
		return Direction::UP;
	}

	if (_playerKeys.left)
	{
		return Direction::LEFT;
	}

	if (_playerKeys.down)
	{
		return Direction::DOWN;
	}

	if (_playerKeys.right)
	{
		return Direction::RIGHT;
	}

	return std::nullopt;
}

bool InputProviderForPlayer::ShouldShoot(Tank& /*self*/) { return _playerKeys.shot; }

void InputProviderForPlayer::Enable()
{
	Subscribe();
}

void InputProviderForPlayer::Disable()
{
	_subs.clear();
}
