#pragma once

#include "Command.h"
#include <string>
#include <boost/serialization/base_object.hpp>
#include <boost/serialization/export.hpp>

class KeyStateChange : public Command
{
	friend class boost::serialization::access;

	std::string _keyState{};
	bool _isEnable{};

public:
	//for deserialization
	KeyStateChange();

	//for serialization
	KeyStateChange(std::string keyState, bool isEnable);

	~KeyStateChange() override = default;

	[[nodiscard]] std::string GetKeyState() const noexcept;
	[[nodiscard]] bool GetIsEnable() const noexcept;

	template<class Archive>
	void serialize(Archive& ar, unsigned int /*version*/);

	[[nodiscard]] const char* GetClassNameW() const noexcept override;
};

template<class Archive>
void KeyStateChange::serialize(Archive& ar, const unsigned int)
{
	ar & boost::serialization::base_object<Command>(*this);
	ar & _keyState;
	ar & _isEnable;
}

BOOST_CLASS_EXPORT_KEY(KeyStateChange);
