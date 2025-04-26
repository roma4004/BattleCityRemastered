#pragma once

#include "../interfaces/IInputProvider.h"

#include <memory>
#include <string>

class EventSystem;

class InputProviderForPlayerTwo final : public IInputProvider
{
	std::string _name{"InputProviderForPlayerTwo"};
	std::shared_ptr<EventSystem> _events{nullptr};

public:
	InputProviderForPlayerTwo(std::shared_ptr<EventSystem> events);

	~InputProviderForPlayerTwo();
};
