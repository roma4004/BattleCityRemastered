#pragma once

#include <string>
#include <boost/serialization/base_object.hpp>
#include <boost/serialization/export.hpp>

#include "Command.h"

class KeyStateChange : public Command
{
	friend class boost::serialization::access;

	std::string _keyState{};

public:
	//for deserialization
	KeyStateChange();

	//for serialization
	KeyStateChange(const std::string& keyState);

	~KeyStateChange() override = default;

	const std::string& GetKeyState() const;

	template<class Archive>
	void serialize(Archive& ar, const unsigned int /*version*/)
	{
		ar & boost::serialization::base_object<Command>(*this);
		ar & _keyState;
	}

	const char* GetClassNameW() const override;
};

BOOST_CLASS_EXPORT_KEY(KeyStateChange);
