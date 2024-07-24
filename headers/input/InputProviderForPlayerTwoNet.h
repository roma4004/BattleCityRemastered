#pragma once

#include "../EventSystem.h"
#include "../interfaces/IInputProvider.h"

#include <memory>

class InputProviderForPlayerTwoNet final : public IInputProvider
{
	std::string _name;
	std::shared_ptr<EventSystem> _events;

public:
	InputProviderForPlayerTwoNet(std::string name, std::shared_ptr<EventSystem> events);

	~InputProviderForPlayerTwoNet();
};
