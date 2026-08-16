#pragma once

#include "enums/ClientSignal.h"
#include "enums/CommandType.h"

namespace network::commands
{
class SignalEvent
{
	CommandType _type{CommandType::SIGNAL_EVENT};
	ClientSignal _signal{};

public:
	//for deserialization
	SignalEvent() = default;

	//for serialization
	explicit SignalEvent(ClientSignal signal);

	[[nodiscard]] CommandType GetType() const noexcept;
	[[nodiscard]] ClientSignal GetSignal() const noexcept;
	[[nodiscard]] const char* GetClassNameW() const noexcept;

	template<class Archive>
	void serialize(Archive& ar, unsigned int /*version*/);
};

template<class Archive>
void SignalEvent::serialize(Archive& ar, const unsigned int)
{
	ar & _type;
	ar & _signal;
}
}//namespace network::commands
