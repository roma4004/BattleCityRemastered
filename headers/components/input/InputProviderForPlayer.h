#pragma once

#include "components/EventSystem.h"
#include "components/input/PlayerKeys.h"
#include "enums/InputChannel.h"
#include "interfaces/IInputProvider.h"
#include <memory>
#include <optional>
#include <vector>

enum class Direction : char8_t;
struct MoveUpEvent;
struct MoveLeftEvent;
struct MoveDownEvent;
struct MoveRightEvent;
struct FireEvent;
class EventSystem;
class Tank;

class InputProviderForPlayer final : public IInputProvider
{
	std::shared_ptr<EventSystem> _events{nullptr};
	std::vector<EventSubscription> _subs{};
	InputChannel _channel;
	PlayerKeys _playerKeys{};

	void OnMoveUp(const MoveUpEvent& event);
	void OnMoveLeft(const MoveLeftEvent& event);
	void OnMoveDown(const MoveDownEvent& event);
	void OnMoveRight(const MoveRightEvent& event);
	void OnFire(const FireEvent& event);

	void Subscribe();

public:
	InputProviderForPlayer(const std::shared_ptr<EventSystem>& events, InputChannel channel);

	~InputProviderForPlayer() override = default;

	[[nodiscard]] std::optional<Direction> ChooseDirection(Tank& self, double deltaTime) override;
	[[nodiscard]] bool ShouldShoot(Tank& self) override;

	void Enable() override;
	void Disable() override;
};
