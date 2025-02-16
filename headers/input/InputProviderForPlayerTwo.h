#pragma once

#include "../interfaces/IInputProvider.h"

#include <memory>
#include <string>

class EventSystem;

class InputProviderForPlayerTwo final : public IInputProvider
{
	std::string _name;
	std::shared_ptr<EventSystem> _events{nullptr};

public:
	InputProviderForPlayerTwo(std::string name, std::shared_ptr<EventSystem> events);

	~InputProviderForPlayerTwo();
};
