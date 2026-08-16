#pragma once

#include "enums/CommandType.h"
#include "enums/InputSignal.h"
#include "enums/PlayerTag.h"

namespace network::commands
{
class KeyStateChange
{
	CommandType _type{CommandType::KEY_STATE_CHANGE};
	PlayerTag _tag{PlayerTag::None};
	InputSignal _action{};
	bool _isPressed{};

public:
	//for deserialization
	KeyStateChange() = default;

	//for serialization
	KeyStateChange(PlayerTag tag, InputSignal action, bool isPressed);

	[[nodiscard]] CommandType GetType() const noexcept;
	[[nodiscard]] PlayerTag GetTag() const noexcept;
	[[nodiscard]] InputSignal GetAction() const noexcept;
	[[nodiscard]] bool GetIsEnable() const noexcept;
	[[nodiscard]] const char* GetClassNameW() const noexcept;

	template<class Archive>
	void serialize(Archive& ar, unsigned int /*version*/);
};

template<class Archive>
void KeyStateChange::serialize(Archive& ar, const unsigned int)
{
	ar & _type;
	ar & _tag;
	ar & _action;
	ar & _isPressed;
}
}//namespace network::commands
