#pragma once

#include <string>
#include <boost/serialization/base_object.hpp>
#include <boost/serialization/export.hpp>
#include <boost/uuid/uuid.hpp>

#include "Command.h"
#include "UuidSerialization.h"

class FortressChange : public Command
{
	friend class boost::serialization::access;

	std::string _state{};
	boost::uuids::uuid _uuid{};

public:
	//for deserialization
	FortressChange();

	//for serialization
	FortressChange(const std::string& state, boost::uuids::uuid uuid);

	~FortressChange() override = default;

	const std::string& GetState() const;
	boost::uuids::uuid GetUuid() const;

	template<class Archive>
	void serialize(Archive& ar, const unsigned int /*version*/)
	{
		ar & boost::serialization::base_object<Command>(*this);
		ar & _state;
		ar & _uuid;
	}

	const char* GetClassNameW() const override;
};

BOOST_CLASS_EXPORT_KEY(FortressChange);
