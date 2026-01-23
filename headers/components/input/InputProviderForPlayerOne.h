#pragma once

#include "interfaces/IInputProvider.h"
#include <memory>
#include <string>

class EventSystem;

class InputProviderForPlayerOne final : public IInputProvider
{
	std::string _name{"InputProviderForPlayerOne"};
	std::shared_ptr<EventSystem> _events{nullptr};

public:
	explicit InputProviderForPlayerOne(const std::shared_ptr<EventSystem>& events);

	~InputProviderForPlayerOne() override;

	void Subscribe();
	void Unsubscribe() const;

	void Enable() override;
	void Disable() const override;
};
