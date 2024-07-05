#pragma once

class ITickUpdatable
{
	virtual void TickUpdate(float deltaTime) = 0;

protected:
	virtual ~ITickUpdatable() = default;
};
