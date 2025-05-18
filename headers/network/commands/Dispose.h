#pragma once

#include <string>
#include <boost/serialization/base_object.hpp>
#include <boost/serialization/export.hpp>
#include <boost/uuid/uuid.hpp>

#include "Command.h"
#include "UuidSerialization.h"

class Dispose : public Command
{
	friend class boost::serialization::access;

	std::string _who{};
	boost::uuids::uuid _uuid{};

public:
	//for deserialization
	Dispose();

	//for serialization
	Dispose(const std::string& who, boost::uuids::uuid uuid);

	~Dispose() override = default;

	const std::string& GetWho() const;
	boost::uuids::uuid GetUuid() const;

	template<class Archive>
	void serialize(Archive& ar, const unsigned int /*version*/)
	{
		ar & boost::serialization::base_object<Command>(*this);
		ar & _who;
		ar & _uuid;
	}

	const char* GetClassNameW() const override;
};

BOOST_CLASS_EXPORT_KEY(Dispose);
