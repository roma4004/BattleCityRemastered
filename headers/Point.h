#pragma once

#include <cmath>
#include <ostream> // for Google Test

struct FPoint final
{
	float x{}, y{};

	template<class Archive>
	void serialize(Archive& ar, unsigned int version);

	[[nodiscard]] bool operator==(const FPoint& rhs) const
	{
		static constexpr float epsilon = 1e-4f;
		return std::abs(x - rhs.x) < epsilon && std::abs(y - rhs.y) < epsilon;
	}

	[[nodiscard]] bool operator!=(const FPoint& rhs) const { return !(*this == rhs); }

	[[nodiscard]] bool operator<(const FPoint& rhs) const
	{
		static constexpr float epsilon = 1e-4f;
		return x < rhs.x && std::abs(x - rhs.x) > epsilon ||
		       (std::abs(x - rhs.x) < epsilon && y < rhs.y && std::abs(y - rhs.y) > epsilon);
	}

	[[nodiscard]] bool operator>(const FPoint& rhs) const { return rhs < *this; }

	[[nodiscard]] bool operator<=(const FPoint& rhs) const { return *this == rhs || *this < rhs; }

	[[nodiscard]] bool operator>=(const FPoint& rhs) const { return *this == rhs || *this > rhs; }
};

struct Point final
{
	int x{}, y{};

	template<class Archive>
	void serialize(Archive& ar, unsigned int version);

	[[nodiscard]] bool operator==(const Point& rhs) const { return x == rhs.x && y == rhs.y; }

	[[nodiscard]] bool operator!=(const Point& rhs) const { return !(*this == rhs); }

	[[nodiscard]] bool operator<(const Point& rhs) const { return x < rhs.x && y < rhs.y; }

	[[nodiscard]] bool operator>(const Point& rhs) const { return rhs < *this; }

	[[nodiscard]] bool operator<=(const Point& rhs) const { return *this == rhs || *this < rhs; }

	[[nodiscard]] bool operator>=(const Point& rhs) const { return *this == rhs || *this > rhs; }
};

struct UPoint final
{
	size_t x{}, y{};

	template<class Archive>
	void serialize(Archive& ar, unsigned int version);

	[[nodiscard]] bool operator==(const UPoint& rhs) const { return x == rhs.x && y == rhs.y; }

	[[nodiscard]] bool operator!=(const UPoint& rhs) const { return !(*this == rhs); }

	[[nodiscard]] bool operator<(const UPoint& rhs) const { return x < rhs.x && y < rhs.y; }

	[[nodiscard]] bool operator>(const UPoint& rhs) const { return rhs < *this; }

	[[nodiscard]] bool operator<=(const UPoint& rhs) const { return *this == rhs || *this < rhs; }

	[[nodiscard]] bool operator>=(const UPoint& rhs) const { return *this == rhs || *this > rhs; }
};

// for Google Test
inline void PrintTo(const FPoint& point, std::ostream* os)
{
	*os << "FPoint(x: " << point.x << ", y: " << point.y << ")";
}

// for Google Test
inline void PrintTo(const Point& point, std::ostream* os)
{
	*os << "Point(x: " << point.x << ", y: " << point.y << ")";
}

// for Google Test
inline void PrintTo(const UPoint& point, std::ostream* os)
{
	*os << "UPoint(x: " << point.x << ", y: " << point.y << ")";
}

// Include the template implementation
#include "Point.tpp"
