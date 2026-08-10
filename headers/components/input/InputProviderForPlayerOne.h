#pragma once

#include "components/EventSystem.h"
#include "interfaces/IInputProvider.h"
#include <memory>
#include <string>
#include <vector>

class EventSystem;
struct MoveUpEvent;
struct MoveLeftEvent;
struct MoveDownEvent;
struct MoveRightEvent;
struct FireEvent;

class InputProviderForPlayerOne final : public IInputProvider
{
	std::string _name{"InputProviderForPlayerOne"};
	std::shared_ptr<EventSystem> _events{nullptr};
	// mutable: Disable() is const (IInputProvider interface) but must be able to unsubscribe.
	mutable std::vector<EventSubscription> _subs{};

	void OnMoveUp(const MoveUpEvent& event);
	void OnMoveLeft(const MoveLeftEvent& event);
	void OnMoveDown(const MoveDownEvent& event);
	void OnMoveRight(const MoveRightEvent& event);
	void OnFire(const FireEvent& event);

public:
	explicit InputProviderForPlayerOne(const std::shared_ptr<EventSystem>& events);

	~InputProviderForPlayerOne() override = default;

	void Subscribe();

	void Enable() override;
	void Disable() const override;
};
