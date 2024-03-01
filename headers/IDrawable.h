#pragma once

#include "../headers/Environment.h"

class Environment;

class IDrawable {
public:
	virtual ~IDrawable() = default;
	virtual void Draw(Environment &env) const = 0;
};
