#pragma once

#include "enums/CommandType.h"
#include <ser20/types/string.hpp>
#include <string>

namespace network::commands
{
class KeyStateChange
{
	CommandType _type{CommandType::KEY_STATE_CHANGE};
	std::string _keyState{};
	bool _isPressed{};

public:
	//for deserialization
	KeyStateChange() = default;

	//for serialization
	KeyStateChange(std::string keyState, bool isPressed);

	[[nodiscard]] CommandType GetType() const noexcept;
	[[nodiscard]] std::string GetKeyState() const noexcept;
	[[nodiscard]] bool GetIsEnable() const noexcept;
	[[nodiscard]] const char* GetClassNameW() const noexcept;

	template<class Archive>
	void serialize(Archive& ar, unsigned int /*version*/);
};

template<class Archive>
void KeyStateChange::serialize(Archive& ar, const unsigned int)
{
	ar & _type;
	ar & _keyState;
	ar & _isPressed;
}
}//namespace network::commands
