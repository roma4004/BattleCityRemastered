#pragma once

#include "EventSystem.h"
#include "IInputProvider.h"

#include <memory>

class InputProviderForPlayerTwo final : public IInputProvider
{
public:
	InputProviderForPlayerTwo(std::string name, std::shared_ptr<EventSystem> events);

	~InputProviderForPlayerTwo();

private:
	std::string _name;
	std::shared_ptr<EventSystem> _events;
};
