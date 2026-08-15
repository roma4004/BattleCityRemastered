#include "network/commands/KeyStateChange.h"

namespace network::commands
{
KeyStateChange::KeyStateChange(std::string keyState, const bool isPressed)
	: _keyState{std::move(keyState)}
	, _isPressed{isPressed} {}

CommandType KeyStateChange::GetType() const noexcept { return _type; }

std::string KeyStateChange::GetKeyState() const noexcept { return _keyState; }

bool KeyStateChange::GetIsEnable() const noexcept { return _isPressed; }

const char* KeyStateChange::GetClassNameW() const noexcept { return "KeyStateChange"; }
}//namespace network::commands
