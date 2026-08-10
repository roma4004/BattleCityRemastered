#pragma once

#include "Command.h"
#include <boost/serialization/base_object.hpp>
#include <boost/serialization/export.hpp>
#include <string>

namespace network::commands
{
class StatisticsChange : public Command
{
	friend class boost::serialization::access;

	std::string _eventName{};
	std::string _author{};
	std::string _fraction{};

public:
	//for deserialization
	StatisticsChange();

	//for serialization
	StatisticsChange(std::string eventName, std::string author, std::string fraction);

	~StatisticsChange() override = default;

	[[nodiscard]] std::string GetEventName() const noexcept;
	[[nodiscard]] std::string GetAuthor() const noexcept;
	[[nodiscard]] std::string GetFraction() const noexcept;

	template<class Archive>
	void serialize(Archive& ar, unsigned int /*version*/);

	[[nodiscard]] const char* GetClassNameW() const noexcept override;
};

template<class Archive>
void StatisticsChange::serialize(Archive& ar, const unsigned int)
{
	ar & boost::serialization::base_object<Command>(*this);
	ar & _eventName;
	ar & _author;
	ar & _fraction;
}
}//namespace network::commands

BOOST_CLASS_EXPORT_KEY(network::commands::StatisticsChange);
