#pragma once

#include <string>
#include <boost/serialization/base_object.hpp>
#include <boost/serialization/export.hpp>

#include "../commands/Command.h"

class FortressChange : public Command
{
	friend class boost::serialization::access;

	std::string _state{};
	int _id{};

public:
	//for deserialization
	FortressChange();

	//for serialization
	FortressChange(const std::string& state, int id);

	~FortressChange() override = default;

	const std::string& GetState() const;
	int GetId() const;

	template<class Archive>
	void serialize(Archive& ar, const unsigned int /*version*/)
	{
		ar & boost::serialization::base_object<Command>(*this);
		ar & _state;
		ar & _id;
	}

	const char* GetClassNameW() const override;
};

BOOST_CLASS_EXPORT_KEY(FortressChange);
