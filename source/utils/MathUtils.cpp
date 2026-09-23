#include "utils/MathUtils.h"
#include <cmath>

bool MathUtils::AreEqualAbsolute(const double a, const double b, const double epsilon) noexcept
{
	return std::fabs(a - b) <= epsilon;
}
