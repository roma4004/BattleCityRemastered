#pragma once

#include "../interfaces/IInputProvider.h"

#include <memory>
#include <string>

class EventSystem;

class InputProviderForPlayerOneNet final : public IInputProvider
{
	std::string _name{"InputProviderForPlayerOneNet"};
	std::shared_ptr<EventSystem> _events{nullptr};

public:
	InputProviderForPlayerOneNet(std::shared_ptr<EventSystem> events);

	~InputProviderForPlayerOneNet();
};
