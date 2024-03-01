#pragma once

#include "../headers/Environment.h"

class IMovable {
public:
	virtual ~IMovable() = default;
	virtual void Move(Environment &env) = 0;

	int speed{1};
};
