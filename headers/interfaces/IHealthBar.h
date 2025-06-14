#pragma once

class IHealthBar
{
protected:
	virtual ~IHealthBar() = default;

	virtual void DrawHealthBar(const BaseObj* obj) const = 0;
};
