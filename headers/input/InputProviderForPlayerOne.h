#pragma once

#include "../interfaces/IInputProvider.h"

#include <memory>
#include <string>

class EventSystem;

class InputProviderForPlayerOne final : public IInputProvider
{
	std::string _name{"InputProviderForPlayerOne"};
	std::shared_ptr<EventSystem> _events{nullptr};

public:
	InputProviderForPlayerOne(std::shared_ptr<EventSystem> events);

	~InputProviderForPlayerOne();
};
