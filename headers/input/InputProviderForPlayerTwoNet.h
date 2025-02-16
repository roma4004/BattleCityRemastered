#pragma once

#include "../interfaces/IInputProvider.h"

#include <memory>
#include <string>

class EventSystem;

class InputProviderForPlayerTwoNet final : public IInputProvider
{
	std::string _name;
	std::shared_ptr<EventSystem> _events{nullptr};

public:
	InputProviderForPlayerTwoNet(std::string name, std::shared_ptr<EventSystem> events);

	~InputProviderForPlayerTwoNet();
};
