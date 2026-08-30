#pragma once

#include "components/EventSystem.h"
#include "enums/InputChannel.h"
#include "interfaces/IInputProvider.h"
#include <memory>
#include <vector>

class EventSystem;
struct MoveUpEvent;
struct MoveLeftEvent;
struct MoveDownEvent;
struct MoveRightEvent;
struct FireEvent;

class InputProviderForPlayer final : public IInputProvider
{
	std::shared_ptr<EventSystem> _events{nullptr};
	std::vector<EventSubscription> _subs{};
	InputChannel _channel;

	void OnMoveUp(const MoveUpEvent& event);
	void OnMoveLeft(const MoveLeftEvent& event);
	void OnMoveDown(const MoveDownEvent& event);
	void OnMoveRight(const MoveRightEvent& event);
	void OnFire(const FireEvent& event);

public:
	InputProviderForPlayer(const std::shared_ptr<EventSystem>& events, InputChannel channel);

	~InputProviderForPlayer() override = default;

	void Subscribe();

	void Enable() override;
	void Disable() override;
};
