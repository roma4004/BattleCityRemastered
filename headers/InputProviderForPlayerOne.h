#pragma once

#include "EventSystem.h"
#include "IInputProvider.h"

#include <memory>

class InputProviderForPlayerOne final : public IInputProvider
{
public:
	InputProviderForPlayerOne(std::string name, std::shared_ptr<EventSystem> events);

	~InputProviderForPlayerOne();

private:
	std::string _name;
	std::shared_ptr<EventSystem> _events;
};
