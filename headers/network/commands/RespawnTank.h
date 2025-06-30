#pragma once

#include "Command.h"
#include "UuidSerialization.h"
#include <boost/serialization/base_object.hpp>
#include <boost/serialization/export.hpp>
#include <boost/uuid/uuid.hpp>

enum class TankType : char8_t;

class RespawnTank : public Command
{
	using buuid = boost::uuids::uuid;

	friend class boost::serialization::access;

	buuid _uuid{};
	TankType _tankType{};

public:
	//for deserialization
	RespawnTank();

	//for serialization
	explicit RespawnTank(TankType tankType, buuid uuid);

	~RespawnTank() override = default;

	[[nodiscard]] TankType GetTankType() const;
	[[nodiscard]] buuid GetUuid() const;

	template<class Archive>
	void serialize(Archive& ar, const unsigned int /*version*/);

	[[nodiscard]] const char* GetClassNameW() const override;
};

template<class Archive>
void RespawnTank::serialize(Archive& ar, const unsigned int)
{
	ar & boost::serialization::base_object<Command>(*this);
	ar & _tankType;
	ar & _uuid;
}

BOOST_CLASS_EXPORT_KEY(RespawnTank);
