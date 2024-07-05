#pragma once

class IObstacle
{
protected:
	bool _isPassable{false};
	bool _isDestructible{false};
	bool _isPenetrable{false};

	virtual ~IObstacle() = default;

public:
	[[nodiscard]] virtual bool GetIsPassable() const = 0;
	virtual void SetIsPassable(bool value) = 0;

	[[nodiscard]] virtual bool GetIsDestructible() const = 0;
	virtual void SetIsDestructible(bool value) = 0;

	[[nodiscard]] virtual bool GetIsPenetrable() const = 0;
	virtual void SetIsPenetrable(bool value) = 0;
};
