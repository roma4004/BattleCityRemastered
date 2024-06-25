#include "../headers/ConfigFailure.h"

#include <iostream>

ConfigFailure::ConfigFailure(std::string errorType, const char* errorDescription)
	: _error{std::move(errorType)}, _description{errorDescription} {}

std::unique_ptr<Game> ConfigFailure::CreateGame()
{
	std::cerr << _error << ": " << _description << '\n';

	return std::make_unique<GameFailure>();
}
