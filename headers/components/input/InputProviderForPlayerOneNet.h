#pragma once

#include "components/EventSystem.h"
#include "interfaces/IInputProvider.h"
#include <memory>
#include <string>
#include <vector>

class EventSystem;

class InputProviderForPlayerOneNet final : public IInputProvider
{
	std::string _name{"InputProviderForPlayerOneNet"};
	std::shared_ptr<EventSystem> _events{nullptr};
	// mutable: Disable() is const (IInputProvider interface) but must be able to unsubscribe.
	mutable std::vector<EventSubscription> _subs{};

public:
	explicit InputProviderForPlayerOneNet(const std::shared_ptr<EventSystem>& events);

	~InputProviderForPlayerOneNet() override = default;

	void Subscribe();

	void Enable() override;
	void Disable() const override;
};
