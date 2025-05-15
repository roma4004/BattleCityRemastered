#pragma once

#include <boost/serialization/access.hpp>
#include <boost/serialization/export.hpp>
#include <boost/serialization/shared_ptr.hpp>

#include "../../enums/ComandType.h"

class Command
{
	friend class boost::serialization::access;

	CommandType _type;

public:
	explicit Command(CommandType type);
	virtual ~Command() = default;

	CommandType GetType() const;

	virtual const char* GetClassNameW() const = 0;

	template<class Archive>
	void serialize(Archive& ar, const unsigned int /*version*/)
	{
		ar & _type;
	}
};

// BOOST_CLASS_EXPORT(Command);
BOOST_SERIALIZATION_ASSUME_ABSTRACT(Command);

