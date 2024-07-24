#pragma once

#include "../EventSystem.h"
#include "../interfaces/IInputProvider.h"

#include <memory>

class InputProviderForPlayerOneNet final : public IInputProvider
{
	std::string _name;
	std::shared_ptr<EventSystem> _events;

public:
	InputProviderForPlayerOneNet(std::string name, std::shared_ptr<EventSystem> events);

	~InputProviderForPlayerOneNet();
};
