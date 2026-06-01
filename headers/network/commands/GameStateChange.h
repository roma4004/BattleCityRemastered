#pragma once

#include "Command.h"
#include <boost/serialization/base_object.hpp>
#include <boost/serialization/export.hpp>
#include <string>

namespace network::commands
{
class GameStateChange : public Command
{
	friend class boost::serialization::access;

	std::string _gameState{};

public:
	//for deserialization
	GameStateChange();

	//for serialization
	explicit GameStateChange(std::string gameState);

	~GameStateChange() override = default;

	[[nodiscard]] std::string GetGameState() const noexcept;

	template<class Archive>
	void serialize(Archive& ar, unsigned int /*version*/);

	[[nodiscard]] const char* GetClassNameW() const noexcept override;
};

template<class Archive>
void GameStateChange::serialize(Archive& ar, const unsigned int)
{
	ar & boost::serialization::base_object<Command>(*this);
	ar & _gameState;
}
}//namespace network::commands

BOOST_CLASS_EXPORT_KEY(network::commands::GameStateChange);
