#pragma once

struct Environment;

class IMovable
{
public:
	virtual ~IMovable() = default;
	virtual void Move(Environment* env) = 0;

	int Speed{1};
};
