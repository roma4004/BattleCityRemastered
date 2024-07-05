#pragma once

class IDrawable
{
	virtual void Draw() const = 0;

protected:
	virtual ~IDrawable() = default;
};
