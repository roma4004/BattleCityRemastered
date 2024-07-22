#pragma once

#include <cmath>

struct FPoint
{
	float x{}, y{};

	template<class Archive>
	void serialize(Archive& ar, const unsigned int version)
	{
		ar & x;
		ar & y;
	}

	bool operator==(const FPoint& rhs) const
	{
		static constexpr float epsilon = 1e-4f;
		return std::abs(x - rhs.x) < epsilon && std::abs(y - rhs.y) < epsilon;
	}

	bool operator!=(const FPoint& rhs) const { return !(*this == rhs); }

	bool operator<(const FPoint& rhs) const
	{
		static constexpr float epsilon = 1e-4f;
		return x < rhs.x && std::abs(x - rhs.x) > epsilon ||
		       (std::abs(x - rhs.x) < epsilon && y < rhs.y && std::abs(y - rhs.y) > epsilon);
	}

	bool operator>(const FPoint& rhs) const { return rhs < *this; }

	bool operator<=(const FPoint& rhs) const { return *this == rhs || *this < rhs; }

	bool operator>=(const FPoint& rhs) const { return *this == rhs || *this > rhs; }
};

struct Point
{
	int x{}, y{};

	template<class Archive>
	void serialize(Archive& ar, const unsigned int version)
	{
		ar & x;
		ar & y;
	}

	bool operator==(const Point& rhs) const { return x == rhs.x && y == rhs.y; }

	bool operator!=(const Point& rhs) const { return !(*this == rhs); }

	bool operator<(const Point& rhs) const { return x < rhs.x && y < rhs.y; }

	bool operator>(const Point& rhs) const { return rhs < *this; }

	bool operator<=(const Point& rhs) const { return *this == rhs || *this < rhs; }

	bool operator>=(const Point& rhs) const { return *this == rhs || *this > rhs; }
};

struct UPoint
{
	size_t x{}, y{};

	template<class Archive>
	void serialize(Archive& ar, const unsigned int version)
	{
		ar & x;
		ar & y;
	}

	bool operator==(const UPoint& rhs) const { return x == rhs.x && y == rhs.y; }

	bool operator!=(const UPoint& rhs) const { return !(*this == rhs); }

	bool operator<(const UPoint& rhs) const { return x < rhs.x && y < rhs.y; }

	bool operator>(const UPoint& rhs) const { return rhs < *this; }

	bool operator<=(const UPoint& rhs) const { return *this == rhs || *this < rhs; }

	bool operator>=(const UPoint& rhs) const { return *this == rhs || *this > rhs; }
};
