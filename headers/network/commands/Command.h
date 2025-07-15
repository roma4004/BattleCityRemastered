#pragma once

#include "enums/ComandType.h"
#include <boost/serialization/access.hpp>
#include <boost/serialization/export.hpp>
#include <boost/serialization/shared_ptr.hpp>

class Command
{
	friend class boost::serialization::access;

	CommandType _type{};

public:
	explicit Command(CommandType type);
	virtual ~Command() = default;

	[[nodiscard]] CommandType GetType() const noexcept;
	[[nodiscard]] virtual const char* GetClassNameW() const noexcept = 0;
	[[nodiscard]] const char* GetCommandType() const noexcept;

	template<class Archive>
	void serialize(Archive& ar, const unsigned int /*version*/);
};

template<class Archive>
void Command::serialize(Archive& ar, const unsigned int)
{
	ar & _type;
}

BOOST_SERIALIZATION_ASSUME_ABSTRACT(Command);
