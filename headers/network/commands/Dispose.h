#pragma once

#include "Command.h"
#include "UuidSerialization.h"
#include <string>
#include <boost/serialization/base_object.hpp>
#include <boost/serialization/export.hpp>
#include <boost/uuid/uuid.hpp>

class Dispose : public Command
{
	using buuid = boost::uuids::uuid;

	friend class boost::serialization::access;

	std::string _who{};
	buuid _uuid{};

public:
	//for deserialization
	Dispose();

	//for serialization
	Dispose(std::string who, buuid uuid);

	~Dispose() override = default;

	[[nodiscard]] std::string GetWho() const noexcept;
	[[nodiscard]] buuid GetUuid() const noexcept;

	template<class Archive>
	void serialize(Archive& ar, const unsigned int /*version*/)
	{
		ar & boost::serialization::base_object<Command>(*this);
		ar & _who;
		ar & _uuid;
	}

	[[nodiscard]] const char* GetClassNameW() const noexcept override;
};

BOOST_CLASS_EXPORT_KEY(Dispose);
