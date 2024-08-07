#pragma once

#include "EventSystem.h"
#include "interfaces/IInputProvider.h"

#include <memory>

class InputProviderForPlayerTwo final : public IInputProvider
{
	std::string _name;
	std::shared_ptr<EventSystem> _events;

public:
	InputProviderForPlayerTwo(std::string name, std::shared_ptr<EventSystem> events);

	~InputProviderForPlayerTwo();
};
