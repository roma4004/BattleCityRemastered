#pragma once

#include "enums/CommandType.h"
#include <ser20/types/string.hpp>
#include <string>

namespace network::commands
{
class GameStateChange
{
	CommandType _type{CommandType::GAME_STATE_CHANGE};
	std::string _gameState{};

public:
	//for deserialization
	GameStateChange() = default;

	//for serialization
	explicit GameStateChange(std::string gameState);

	[[nodiscard]] CommandType GetType() const noexcept;
	[[nodiscard]] std::string GetGameState() const noexcept;
	[[nodiscard]] const char* GetClassNameW() const noexcept;

	template<class Archive>
	void serialize(Archive& ar, unsigned int /*version*/);
};

template<class Archive>
void GameStateChange::serialize(Archive& ar, const unsigned int)
{
	ar & _type;
	ar & _gameState;
}
}//namespace network::commands
