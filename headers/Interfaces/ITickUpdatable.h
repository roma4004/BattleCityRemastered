#pragma once

class ITickUpdatable
{
public:
	virtual ~ITickUpdatable() = default;
	virtual void TickUpdate(float deltaTime) = 0;
};
