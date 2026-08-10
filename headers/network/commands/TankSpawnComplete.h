#pragma once

#include "Command.h"
#include "UuidSerialization.h"
#include <boost/serialization/base_object.hpp>
#include <boost/serialization/export.hpp>
#include <boost/uuid/uuid.hpp>

namespace network::commands
{
// Host -> client: "materialize tank uuid now".
class TankSpawnComplete : public Command
{
	using buuid = boost::uuids::uuid;

	friend class boost::serialization::access;

	buuid _uuid{};

public:
	//for deserialization
	TankSpawnComplete();

	//for serialization
	explicit TankSpawnComplete(buuid uuid);

	~TankSpawnComplete() override = default;

	[[nodiscard]] buuid GetUuid() const noexcept;

	template<class Archive>
	void serialize(Archive& ar, unsigned int /*version*/);

	[[nodiscard]] const char* GetClassNameW() const noexcept override;
};

template<class Archive>
void TankSpawnComplete::serialize(Archive& ar, const unsigned int)
{
	ar & boost::serialization::base_object<Command>(*this);
	ar & _uuid;
}
}//namespace network::commands

BOOST_CLASS_EXPORT_KEY(network::commands::TankSpawnComplete);
