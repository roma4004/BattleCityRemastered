#pragma once

#include "Command.h"
#include <string>
#include <boost/serialization/base_object.hpp>
#include <boost/serialization/export.hpp>

class KeyStateChange : public Command
{
	friend class boost::serialization::access;

	std::string _keyState{};

public:
	//for deserialization
	KeyStateChange();

	//for serialization
	KeyStateChange(std::string keyState);

	~KeyStateChange() override = default;

	[[nodiscard]] std::string GetKeyState() const noexcept;

	template<class Archive>
	void serialize(Archive& ar, const unsigned int /*version*/);

	[[nodiscard]] const char* GetClassNameW() const noexcept override;
};

template<class Archive>
void KeyStateChange::serialize(Archive& ar, const unsigned int)
{
	ar & boost::serialization::base_object<Command>(*this);
	ar & _keyState;
}

BOOST_CLASS_EXPORT_KEY(KeyStateChange);
