#pragma once

#include "components/EventSystem.h"
#include "interfaces/IInputProvider.h"
#include <memory>
#include <vector>

class EventSystem;
struct ServerInMoveUpEvent;
struct ServerInMoveLeftEvent;
struct ServerInMoveDownEvent;
struct ServerInMoveRightEvent;
struct ServerInFireEvent;
struct ServerInPauseReleasedEvent;

class InputProviderForPlayerOneNet final : public IInputProvider
{
	std::shared_ptr<EventSystem> _events{nullptr};
	// mutable: Disable() is const (IInputProvider interface) but must be able to unsubscribe.
	mutable std::vector<EventSubscription> _subs{};

	void OnMoveUp(const ServerInMoveUpEvent& event);
	void OnMoveLeft(const ServerInMoveLeftEvent& event);
	void OnMoveDown(const ServerInMoveDownEvent& event);
	void OnMoveRight(const ServerInMoveRightEvent& event);
	void OnFire(const ServerInFireEvent& event);
	void OnPauseReleased(const ServerInPauseReleasedEvent&) const;

public:
	explicit InputProviderForPlayerOneNet(const std::shared_ptr<EventSystem>& events);

	~InputProviderForPlayerOneNet() override = default;

	void Subscribe();

	void Enable() override;
	void Disable() const override;
};
