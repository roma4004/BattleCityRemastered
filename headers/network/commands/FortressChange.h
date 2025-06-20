#pragma once

#include "Command.h"
#include "UuidSerialization.h"
#include <string>
#include <boost/serialization/base_object.hpp>
#include <boost/serialization/export.hpp>
#include <boost/uuid/uuid.hpp>

class FortressChange : public Command
{
	using buuid = boost::uuids::uuid;

	friend class boost::serialization::access;

	std::string _state{};
	buuid _uuid{};

public:
	//for deserialization
	FortressChange();

	//for serialization
	FortressChange(const std::string& state, buuid uuid);

	~FortressChange() override = default;

	const std::string& GetState() const;
	buuid GetUuid() const;

	template<class Archive>
	void serialize(Archive& ar, const unsigned int /*version*/);

	const char* GetClassNameW() const override;
};

template<class Archive>
void FortressChange::serialize(Archive& ar, const unsigned int)
{
	ar & boost::serialization::base_object<Command>(*this);
	ar & _state;
	ar & _uuid;
}

BOOST_CLASS_EXPORT_KEY(FortressChange);
