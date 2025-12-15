#pragma once

#include "Command.h"
#include "UuidSerialization.h"
#include <boost/serialization/base_object.hpp>
#include <boost/serialization/export.hpp>
#include <boost/uuid/uuid.hpp>

class TankOnOff : public Command
{
	using buuid = boost::uuids::uuid;

	friend class boost::serialization::access;

	buuid _uuid{};
	bool _isEnable{};
	std::string _name{};

public:
	//for deserialization
	TankOnOff();

	//for serialization
	explicit TankOnOff(buuid uuid, bool isEnable, std::string name);

	~TankOnOff() override = default;

	[[nodiscard]] buuid GetUuid() const noexcept;
	[[nodiscard]] bool GetIsEnable() const noexcept;
	[[nodiscard]] std::string GetName() const noexcept;

	template<class Archive>
	void serialize(Archive& ar, unsigned int /*version*/);

	[[nodiscard]] const char* GetClassNameW() const noexcept override;
};

template<class Archive>
void TankOnOff::serialize(Archive& ar, const unsigned int)
{
	ar & boost::serialization::base_object<Command>(*this);
	ar & _uuid;
	ar & _isEnable;
	ar & _name;
}

BOOST_CLASS_EXPORT_KEY(TankOnOff);
