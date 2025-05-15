#pragma once

#include <string>
#include <boost/serialization/base_object.hpp>
#include <boost/serialization/export.hpp>

#include "../../Point.h"
#include "../../enums/Direction.h"
#include "../commands/Command.h"

class PositionChange : public Command
{
	friend class boost::serialization::access;

	std::string _who{};
	FPoint _pos{};
	Direction _dir{};

public:
	//for deserialization
	PositionChange();

	//for serialization
	PositionChange(const std::string& who, const FPoint& pos, Direction dir);

	~PositionChange() override = default;

	const std::string& GetWho() const;
	FPoint GetPos() const;
	Direction GetDir() const;

	template<class Archive>
	void serialize(Archive& ar, const unsigned int /*version*/)
	{
		ar & boost::serialization::base_object<Command>(*this);
		ar & _who;
		ar & _pos;
		ar & _dir;
	}

	const char* GetClassNameW() const override;
};

BOOST_CLASS_EXPORT_KEY(PositionChange);
