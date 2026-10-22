#pragma once

#include <optional>
#include <vector>

enum class Direction : char8_t;

//NOTE: d-pad and left stick drive one tank - the last pressed direction wins, releasing it returns to a held one
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
