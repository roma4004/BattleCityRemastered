#pragma once

#include "enums/CommandType.h"
#include "enums/DisconnectReason.h"

namespace network::commands
{
class Disconnect
{
	CommandType _type{CommandType::DISCONNECT};
	DisconnectReason _reason{};

public:
	//for deserialization
	Disconnect() = default;

	//for serialization
	explicit Disconnect(DisconnectReason reason);

	[[nodiscard]] CommandType GetType() const noexcept;
	[[nodiscard]] DisconnectReason GetReason() const noexcept;
	[[nodiscard]] const char* GetClassNameW() const noexcept;

	template<class Archive>
	void serialize(Archive& ar, unsigned int /*version*/);
};

template<class Archive>
void Disconnect::serialize(Archive& ar, const unsigned int)
{
	ar & _type;
	ar & _reason;
}
}//namespace network::commands
