#include "network/commands/KeyStateChange.h"

namespace network::commands
{
KeyStateChange::KeyStateChange(const PlayerTag tag, const InputSignal action, const bool isPressed)
	: _tag{tag}
	, _action{action}
	, _isPressed{isPressed} {}

CommandType KeyStateChange::GetType() const noexcept { return _type; }

PlayerTag KeyStateChange::GetTag() const noexcept { return _tag; }

InputSignal KeyStateChange::GetAction() const noexcept { return _action; }

bool KeyStateChange::GetIsEnable() const noexcept { return _isPressed; }

const char* KeyStateChange::GetClassNameW() const noexcept { return "KeyStateChange"; }
}//namespace network::commands
