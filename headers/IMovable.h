#pragma once

class Environment;

class IMovable {
public:
	virtual ~IMovable() = default;
	virtual void Move(Environment& env) = 0;

	int speed{1};
};
