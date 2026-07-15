#include "application/ConfigFailure.h"
#include "application/GameFailure.h"
#include "enums/GameMode.h"
#include <iostream>

ConfigFailure::ConfigFailure(std::string errorType, const char* errorDescription)
	: _error{std::move(errorType)}
	, _description{errorDescription} {}

std::unique_ptr<IGame> ConfigFailure::CreateGame(const GameMode /*gameMode*/, SDL_Config& /*sdlConfig*/)
{
	std::cerr << _error << ": " << _description << '\n';

	return std::make_unique<GameFailure>();
}
