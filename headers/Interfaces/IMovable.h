#pragma once

class IMovable
{
public:
	virtual ~IMovable() = default;
	virtual void Move(float deltaTime) = 0;
};
