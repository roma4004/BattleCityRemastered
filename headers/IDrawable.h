#pragma once

struct Environment;

class IDrawable
{
public:
	virtual ~IDrawable() = default;
	virtual void Draw(const Environment* env) const = 0;
};
