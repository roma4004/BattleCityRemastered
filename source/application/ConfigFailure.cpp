#include "../../headers/application/ConfigFailure.h"
#include "../../headers/application/GameFailure.h"

#include <iostream>

ConfigFailure::ConfigFailure(std::string errorType, const char* errorDescription)
	: _error{std::move(errorType)}, _description{errorDescription} {}

std::unique_ptr<IGame> ConfigFailure::CreateGame()
{
	std::cerr << _error << ": " << _description << '\n';

	return std::make_unique<GameFailure>();
}
