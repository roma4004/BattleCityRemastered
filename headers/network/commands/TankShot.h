#pragma once

#include "Command.h"
#include "UuidSerialization.h"
#include "enums/Direction.h"
#include <boost/serialization/base_object.hpp>
#include <boost/serialization/export.hpp>
#include <boost/uuid/uuid.hpp>
#include <string>

namespace network::commands
{
class TankShot : public Command
{
	using buuid = boost::uuids::uuid;

	friend class boost::serialization::access;

	std::string _who{};
	Direction _dir{};
	buuid _uuid{};

public:
	//for deserialization
	TankShot();

	//for serialization
	TankShot(std::string who, Direction dir, buuid uuid);

	~TankShot() override = default;

	[[nodiscard]] std::string GetWho() const noexcept;
	[[nodiscard]] Direction GetDir() const noexcept;
	[[nodiscard]] buuid GetUuid() const noexcept;

	template<class Archive>
	void serialize(Archive& ar, unsigned int /*version*/);

	[[nodiscard]] const char* GetClassNameW() const noexcept override;
};

template<class Archive>
void TankShot::serialize(Archive& ar, const unsigned int)
{
	ar & boost::serialization::base_object<Command>(*this);
	ar & _who;
	ar & _dir;
	ar & _uuid;
}
}//namespace network::commands

BOOST_CLASS_EXPORT_KEY(network::commands::TankShot);
