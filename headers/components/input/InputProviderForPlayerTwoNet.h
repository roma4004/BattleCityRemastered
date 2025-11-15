#pragma once

#include "interfaces/IInputProvider.h"
#include <memory>
#include <string>

class EventSystem;

class InputProviderForPlayerTwoNet final : public IInputProvider
{
	std::string _name{"InputProviderForPlayerTwoNet"};
	std::shared_ptr<EventSystem> _events{nullptr};

public:
	explicit InputProviderForPlayerTwoNet(const std::shared_ptr<EventSystem>& events);

	~InputProviderForPlayerTwoNet() override;

	void Subscribe();
	void Unsubscribe() const;

	void Enable() override;
	void Disable() const override;
};
