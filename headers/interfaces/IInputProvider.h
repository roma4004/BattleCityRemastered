#pragma once

#include <optional>

enum class Direction : char8_t;
class Tank;

//NOTE: who drives the tank - two answers every driver owes, and three hooks it may leave alone
class IInputProvider
{
public:
	virtual ~IInputProvider() = default;

	//NOTE: nullopt means "stay put"; a bot names its current direction when it sees no reason to turn
	[[nodiscard]] virtual std::optional<Direction> ChooseDirection(Tank& self, double deltaTime) = 0;

	//NOTE: asked every frame, cooldown or not - a bot aims during this pass
	[[nodiscard]] virtual bool ShouldShoot(Tank& self) = 0;

	//NOTE: asked after a blocked move; a player keeps holding the key into the wall, so only a bot revises
	[[nodiscard]] virtual std::optional<Direction> ReviseWhenMoveBlocked(Tank& /*self*/, double /*deltaTime*/)
	{
		return std::nullopt;
	}

	//NOTE: a bot reads the field and has nothing to subscribe to
	virtual void Enable() {}
	virtual void Disable() {}
};
