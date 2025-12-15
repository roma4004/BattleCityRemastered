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

	TankType _tankType{};
	buuid _uuid{};

public:
	//for deserialization
	RespawnTank();

	//for serialization
	explicit RespawnTank(TankType tankType, buuid uuid);

	~RespawnTank() override = default;

	[[nodiscard]] TankType GetTankType() const noexcept;
	[[nodiscard]] buuid GetUuid() const noexcept;

	template<class Archive>
	void serialize(Archive& ar, unsigned int /*version*/);

	[[nodiscard]] const char* GetClassNameW() const noexcept override;
};

template<class Archive>
void RespawnTank::serialize(Archive& ar, const unsigned int)
{
	ar & boost::serialization::base_object<Command>(*this);
	ar & _tankType;
	ar & _uuid;
}

BOOST_CLASS_EXPORT_KEY(RespawnTank);
