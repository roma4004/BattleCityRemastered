#include "network/commands/SignalEvent.h"

namespace network::commands
{
SignalEvent::SignalEvent(const ClientSignal signal)
	: _signal{signal} {}

CommandType SignalEvent::GetType() const noexcept { return _type; }

ClientSignal SignalEvent::GetSignal() const noexcept { return _signal; }

const char* SignalEvent::GetClassNameW() const noexcept { return "SignalEvent"; }
}//namespace network::commands
