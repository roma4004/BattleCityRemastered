#pragma once

struct Environment;

class IDestroyable
{
public:
	virtual ~IDestroyable() = default;
	virtual void MarkDestroy(Environment* env) const = 0;
};