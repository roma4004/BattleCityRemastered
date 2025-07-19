#pragma once

class BaseObj;

class IDrawable
{
protected:
	virtual ~IDrawable() = default;

public:
	virtual void Draw() const = 0;
};
