#pragma once

class IObstacle
{
protected:
	bool _isAlive{true};

	virtual ~IObstacle() = default;

public:
	[[nodiscard]] virtual bool GetIsPassable() const = 0;

	[[nodiscard]] virtual bool GetIsDestructible() const = 0;

	[[nodiscard]] virtual bool GetIsPenetrable() const = 0;

	[[nodiscard]] virtual bool GetIsAlive() const = 0;
	virtual void SetIsAlive(bool isAlive) = 0;
};
