#include "network/commands/SignalEvent.h"

namespace network::commands
{
SignalEvent::SignalEvent(std::string signalName)
	: _signalName{std::move(signalName)} {}

CommandType SignalEvent::GetType() const noexcept { return _type; }

std::string SignalEvent::GetSignalName() const noexcept { return _signalName; }

const char* SignalEvent::GetClassNameW() const noexcept { return "SignalEvent"; }
}//namespace network::commands
