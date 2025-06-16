#include "../../../headers/network/commands/KeyStateChange.h"

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/export.hpp>
#include <boost/serialization/shared_ptr.hpp>

BOOST_CLASS_EXPORT_IMPLEMENT(KeyStateChange);

KeyStateChange::KeyStateChange(): Command(CommandType::STATISTICS_CHANGE) {}

KeyStateChange::KeyStateChange(const std::string& keyState)
	: Command(CommandType::STATISTICS_CHANGE), _keyState(keyState) {}

const std::string& KeyStateChange::GetKeyState() const { return _keyState; }

const char* KeyStateChange::GetClassNameW() const { return "KeyStateChange"; }
