#include "network/commands/SignalEvent.h"
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/export.hpp>
#include <boost/serialization/shared_ptr.hpp>

BOOST_CLASS_EXPORT_IMPLEMENT(network::commands::SignalEvent)

namespace network::commands
{
SignalEvent::SignalEvent()
	: Command{CommandType::SIGNAL_EVENT} {}

SignalEvent::SignalEvent(std::string signalName)
	: Command{CommandType::SIGNAL_EVENT}
	, _signalName{std::move(signalName)} {}

std::string SignalEvent::GetSignalName() const noexcept { return _signalName; }

const char* SignalEvent::GetClassNameW() const noexcept { return "SignalEvent"; }
}//namespace network::commands
