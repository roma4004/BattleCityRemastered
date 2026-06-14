#pragma once

class BaseObj;

class IDrawable
{
protected:
	virtual ~IDrawable() = default;
	virtual void Draw() const = 0;
};
