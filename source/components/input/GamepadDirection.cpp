#include "components/input/GamepadDirection.h"
#include "enums/Direction.h"
#include <algorithm>
#include <cstdlib>

GamepadDirection::GamepadDirection(const int deadZone) noexcept
	: _deadZone{deadZone} {}

void GamepadDirection::PressDpad(const Direction dir)
{
	std::erase(_dpad, dir);
	_dpad.push_back(dir);
	_isStickNewest = false;
}

void GamepadDirection::ReleaseDpad(const Direction dir) { std::erase(_dpad, dir); }

void GamepadDirection::MoveStickX(const int value)
{
	_stickX = value;
	UpdateStick();
}

void GamepadDirection::MoveStickY(const int value)
{
	_stickY = value;
	UpdateStick();
}

//NOTE: the dominant axis names the side; a wobble within the same side is not a new press
void GamepadDirection::UpdateStick()
{
	const int x{std::abs(_stickX)};
	const int y{std::abs(_stickY)};

	std::optional<Direction> stick{};
	if (std::max(x, y) > _deadZone)
	{
		stick = x > y ? (_stickX < 0 ? Direction::LEFT : Direction::RIGHT)
					  : (_stickY < 0 ? Direction::UP : Direction::DOWN);
	}

	if (stick == _stick)
	{
		return;
	}

	_stick = stick;
	_isStickNewest = stick.has_value();
}

std::optional<Direction> GamepadDirection::Held() const noexcept
{
	if (_dpad.empty() || (_stick && _isStickNewest))
	{
		return _stick;
	}

	return _dpad.back();
}
