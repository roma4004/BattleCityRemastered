#pragma once

struct Environment;

class ITickUpdatable
{
public:
	virtual ~ITickUpdatable() = default;
	virtual void TickUpdate(Environment* env) = 0;
};
