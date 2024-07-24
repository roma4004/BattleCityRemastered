#pragma once

#include "../EventSystem.h"
#include "../interfaces/IInputProvider.h"

#include <memory>

class InputProviderForPlayerOne final : public IInputProvider
{
	std::string _name;
	std::shared_ptr<EventSystem> _events;

public:
	InputProviderForPlayerOne(std::string name, std::shared_ptr<EventSystem> events);

	~InputProviderForPlayerOne();
};
