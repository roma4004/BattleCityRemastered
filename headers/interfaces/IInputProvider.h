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

	//NOTE: empty, not pure - the only decision taken after the move, and a player who drove into a
	//wall goes on holding the key, so a keyboard has nothing to revise
	[[nodiscard]] virtual std::optional<Direction> ReviseWhenMoveBlocked(Tank& /*self*/, double /*deltaTime*/)
	{
		return std::nullopt;
	}

	//NOTE: empty, not pure - a bot reads the field, so there is nothing for it to subscribe to
	virtual void Enable() {}
	virtual void Disable() {}
};
