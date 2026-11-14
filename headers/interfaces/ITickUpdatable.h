#pragma once

class ITickUpdatable
{
	virtual void TickUpdate(double deltaTime) = 0;

protected:
	virtual ~ITickUpdatable() = default;
};
