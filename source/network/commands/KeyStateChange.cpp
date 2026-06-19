#include "network/commands/KeyStateChange.h"
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/export.hpp>
#include <boost/serialization/shared_ptr.hpp>

BOOST_CLASS_EXPORT_IMPLEMENT(network::commands::KeyStateChange)

namespace network::commands
{
KeyStateChange::KeyStateChange()
	: Command{CommandType::KEY_STATE_CHANGE} {}

KeyStateChange::KeyStateChange(std::string keyState, const bool isPressed)
	: Command{CommandType::KEY_STATE_CHANGE}
	, _keyState{std::move(keyState)}
	, _isPressed{isPressed} {}

std::string KeyStateChange::GetKeyState() const noexcept { return _keyState; }

bool KeyStateChange::GetIsEnable() const noexcept { return _isPressed; }

const char* KeyStateChange::GetClassNameW() const noexcept { return "KeyStateChange"; }
}//namespace network::commands
