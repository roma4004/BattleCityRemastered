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

class InputProviderForPlayerTwoNet final : public IInputProvider
{
	std::shared_ptr<EventSystem> _events{nullptr};
	std::vector<EventSubscription> _subs{};

	void OnMoveUp(const ServerInMoveUpEvent& event);
	void OnMoveLeft(const ServerInMoveLeftEvent& event);
	void OnMoveDown(const ServerInMoveDownEvent& event);
	void OnMoveRight(const ServerInMoveRightEvent& event);
	void OnFire(const ServerInFireEvent& event);

public:
	explicit InputProviderForPlayerTwoNet(const std::shared_ptr<EventSystem>& events);

	~InputProviderForPlayerTwoNet() override = default;

	void Subscribe();

	void Enable() override;
	void Disable() override;
};
