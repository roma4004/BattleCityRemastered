#pragma once

class ITickUpdatable
{
protected:
	virtual void TickUpdate(double deltaTime) = 0;

	virtual ~ITickUpdatable() = default;
};
