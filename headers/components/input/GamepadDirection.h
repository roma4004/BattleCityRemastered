#pragma once

#include <optional>
#include <vector>

enum class Direction : char8_t;

//NOTE: the d-pad and the left stick drive one tank - the direction pressed last wins, and letting it go hands
//the tank back to whatever is still held
class GamepadDirection final
{
public:
	explicit GamepadDirection(int deadZone) noexcept;

	void PressDpad(Direction dir);
	void ReleaseDpad(Direction dir);
	void MoveStickX(int value);
	void MoveStickY(int value);

	[[nodiscard]] std::optional<Direction> Held() const noexcept;

private:
	int _deadZone;
	int _stickX{};
	int _stickY{};
	std::optional<Direction> _stick{};
	std::vector<Direction> _dpad{};
	bool _isStickNewest{};

	void UpdateStick();
};
