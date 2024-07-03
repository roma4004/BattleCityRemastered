#pragma once

class IHealthBar
{
public:
	virtual ~IHealthBar() = default;
	virtual void DrawHealthBar() const = 0;
};
