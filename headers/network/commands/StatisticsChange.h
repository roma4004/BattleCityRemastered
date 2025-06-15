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

	const std::string& GetEventName() const;
	const std::string& GetAuthor() const;
	const std::string& GetFraction() const;

	template<class Archive>
	void serialize(Archive& ar, const unsigned int /*version*/)
	{
		ar & boost::serialization::base_object<Command>(*this);
		ar & _eventName;
		ar & _author;
		ar & _fraction;
	}

	const char* GetClassNameW() const override;
};

BOOST_CLASS_EXPORT_KEY(StatisticsChange);
