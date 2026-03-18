#pragma once

#include "Command.h"
#include <boost/serialization/base_object.hpp>
#include <boost/serialization/export.hpp>
#include <string>

namespace network::commands
{
class SignalEvent : public Command
{
	friend class boost::serialization::access;

	std::string _signalName{};

public:
	//for deserialization
	SignalEvent();

	//for serialization
	SignalEvent(std::string signalName);

	~SignalEvent() override = default;

	[[nodiscard]] std::string GetSignalName() const noexcept;

	template<class Archive>
	void serialize(Archive& ar, unsigned int /*version*/);

	[[nodiscard]] const char* GetClassNameW() const noexcept override;
};

template<class Archive>
void SignalEvent::serialize(Archive& ar, const unsigned int)
{
	ar & boost::serialization::base_object<Command>(*this);
	ar & _signalName;
}
}//namespace network::commands

BOOST_CLASS_EXPORT_KEY(network::commands::SignalEvent);
