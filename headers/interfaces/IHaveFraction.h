#pragma once

#include <string>

class IHaveFraction
{
protected:
	virtual ~IHaveFraction() = default;

public:
	[[nodiscard]] virtual std::string GetFraction() const = 0;
};
