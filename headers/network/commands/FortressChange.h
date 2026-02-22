#pragma once

#include "Command.h"
#include "UuidSerialization.h"
#include <boost/serialization/base_object.hpp>
#include <boost/serialization/export.hpp>
#include <boost/uuid/uuid.hpp>
#include <string>

namespace network::commands
{
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
	FortressChange(std::string state, buuid uuid);

	~FortressChange() override = default;

	[[nodiscard]] std::string GetState() const noexcept;
	[[nodiscard]] buuid GetUuid() const noexcept;

	template<class Archive>
	void serialize(Archive& ar, unsigned int /*version*/);

	[[nodiscard]] const char* GetClassNameW() const noexcept override;
};

template<class Archive>
void FortressChange::serialize(Archive& ar, const unsigned int)
{
	ar & boost::serialization::base_object<Command>(*this);
	ar & _state;
	ar & _uuid;
}
}//namespace network::commands

BOOST_CLASS_EXPORT_KEY(network::commands::FortressChange);
