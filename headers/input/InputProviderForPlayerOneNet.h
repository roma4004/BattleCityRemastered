#pragma once

#include "../interfaces/IInputProvider.h"

#include <memory>
#include <string>

class EventSystem;

class InputProviderForPlayerOneNet final : public IInputProvider
{
	std::string _name;
	std::shared_ptr<EventSystem> _events;

public:
	InputProviderForPlayerOneNet(std::string name, std::shared_ptr<EventSystem> events);

	~InputProviderForPlayerOneNet();
};
