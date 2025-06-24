#pragma once

#include "Command.h"
#include "UuidSerialization.h"
#include "enums/Direction.h"
#include <string>
#include <boost/serialization/base_object.hpp>
#include <boost/serialization/export.hpp>
#include <boost/uuid/uuid.hpp>

class TankShot : public Command
{
	using buuid = boost::uuids::uuid;

	friend class boost::serialization::access;

	std::string _who{};
	buuid _uuid{};
	Direction _dir{};

public:
	//for deserialization
	TankShot();

	//for serialization
	TankShot(const std::string& who, Direction dir, buuid uuid);

	~TankShot() override = default;

	[[nodiscard]] const std::string& GetWho() const;
	[[nodiscard]] Direction GetDir() const;
	[[nodiscard]] buuid GetUuid() const;

	template<class Archive>
	void serialize(Archive& ar, const unsigned int /*version*/);

	[[nodiscard]] const char* GetClassNameW() const override;
};

template<class Archive>
void TankShot::serialize(Archive& ar, const unsigned int)
{
	ar & boost::serialization::base_object<Command>(*this);
	ar & _who;
	ar & _dir;
	ar & _uuid;
}

BOOST_CLASS_EXPORT_KEY(TankShot);
