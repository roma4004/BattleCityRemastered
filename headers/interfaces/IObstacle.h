#pragma once

class IObstacle
{
protected:
	bool _isAlive{true};

	virtual ~IObstacle() = default;

public:
	[[nodiscard]] virtual bool GetIsPassable() const noexcept = 0;

	[[nodiscard]] virtual bool GetIsDestructible() const noexcept = 0;

	[[nodiscard]] virtual bool GetIsPenetrable() const noexcept = 0;

	[[nodiscard]] virtual bool GetIsAlive() const noexcept = 0;
	virtual void SetIsAlive(bool isAlive) noexcept = 0;
};
