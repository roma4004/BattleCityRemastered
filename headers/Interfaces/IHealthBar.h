#pragma once

class IHealthBar
{
protected:
	virtual ~IHealthBar() = default;

	virtual void DrawHealthBar() const = 0;
};
