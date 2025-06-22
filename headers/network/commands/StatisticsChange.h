#pragma once

#include "Command.h"
#include <string>
#include <boost/serialization/base_object.hpp>
#include <boost/serialization/export.hpp>

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
	StatisticsChange(const std::string& eventName, const std::string& author, const std::string& fraction);

	~StatisticsChange() override = default;

	[[nodiscard]] const std::string& GetEventName() const;
	[[nodiscard]] const std::string& GetAuthor() const;
	[[nodiscard]] const std::string& GetFraction() const;

	template<class Archive>
	void serialize(Archive& ar, const unsigned int /*version*/);

	[[nodiscard]] const char* GetClassNameW() const override;
};

template<class Archive>
void StatisticsChange::serialize(Archive& ar, const unsigned int)
{
	ar & boost::serialization::base_object<Command>(*this);
	ar & _eventName;
	ar & _author;
	ar & _fraction;
}

BOOST_CLASS_EXPORT_KEY(StatisticsChange);
