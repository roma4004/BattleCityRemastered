#pragma once

#include "enums/CommandType.h"
#include <ser20/types/string.hpp>
#include <string>

namespace network::commands
{
class SignalEvent
{
	CommandType _type{CommandType::SIGNAL_EVENT};
	std::string _signalName{};

public:
	//for deserialization
	SignalEvent() = default;

	//for serialization
	explicit SignalEvent(std::string signalName);

	[[nodiscard]] CommandType GetType() const noexcept;
	[[nodiscard]] std::string GetSignalName() const noexcept;
	[[nodiscard]] const char* GetClassNameW() const noexcept;

	template<class Archive>
	void serialize(Archive& ar, unsigned int /*version*/);
};

template<class Archive>
void SignalEvent::serialize(Archive& ar, const unsigned int)
{
	ar & _type;
	ar & _signalName;
}
}//namespace network::commands
