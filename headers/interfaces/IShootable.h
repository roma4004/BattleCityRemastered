#pragma once

class IShootable
{
protected:
	virtual ~IShootable() = default;

public:
	virtual void Shot() const = 0;
};
