#pragma once

#include <ostream>// for Google Test

struct FPoint final
{
	float x{}, y{};

	template<class Archive>
	void serialize(Archive& ar, unsigned int version);

	[[nodiscard]] bool operator==(const FPoint& rhs) const noexcept
	{
		static constexpr float epsilon = 1e-4f;
		return std::abs(x - rhs.x) < epsilon && std::abs(y - rhs.y) < epsilon;
	}

	[[nodiscard]] bool operator!=(const FPoint& rhs) const noexcept { return !(*this == rhs); }

	[[nodiscard]] bool operator<(const FPoint& rhs) const noexcept
	{
		static constexpr float epsilon = 1e-4f;
		return (x < rhs.x && std::abs(x - rhs.x) > epsilon) ||
			   (std::abs(x - rhs.x) < epsilon && y < rhs.y && std::abs(y - rhs.y) > epsilon);
	}

	[[nodiscard]] bool operator>(const FPoint& rhs) const noexcept { return rhs < *this; }

	[[nodiscard]] bool operator<=(const FPoint& rhs) const noexcept { return *this == rhs || *this < rhs; }

	[[nodiscard]] bool operator>=(const FPoint& rhs) const noexcept { return *this == rhs || *this > rhs; }
};

struct Point final
{
	int x{}, y{};

	template<class Archive>
	void serialize(Archive& ar, unsigned int version);

	[[nodiscard]] bool operator==(const Point& rhs) const noexcept { return x == rhs.x && y == rhs.y; }

	[[nodiscard]] bool operator!=(const Point& rhs) const noexcept { return !(*this == rhs); }

	[[nodiscard]] bool operator<(const Point& rhs) const noexcept { return x < rhs.x && y < rhs.y; }

	[[nodiscard]] bool operator>(const Point& rhs) const noexcept { return rhs < *this; }

	[[nodiscard]] bool operator<=(const Point& rhs) const noexcept { return *this == rhs || *this < rhs; }

	[[nodiscard]] bool operator>=(const Point& rhs) const noexcept { return *this == rhs || *this > rhs; }
};

struct UPoint final
{
	size_t x{}, y{};

	template<class Archive>
	void serialize(Archive& ar, unsigned int version);

	[[nodiscard]] bool operator==(const UPoint& rhs) const noexcept { return x == rhs.x && y == rhs.y; }

	[[nodiscard]] bool operator!=(const UPoint& rhs) const noexcept { return !(*this == rhs); }

	[[nodiscard]] bool operator<(const UPoint& rhs) const noexcept { return x < rhs.x && y < rhs.y; }

	[[nodiscard]] bool operator>(const UPoint& rhs) const noexcept { return rhs < *this; }

	[[nodiscard]] bool operator<=(const UPoint& rhs) const noexcept { return *this == rhs || *this < rhs; }

	[[nodiscard]] bool operator>=(const UPoint& rhs) const noexcept { return *this == rhs || *this > rhs; }
};

template<class Archive>
void FPoint::serialize(Archive& ar, const unsigned int /*version*/)
{
	ar & x;
	ar & y;
}

template<class Archive>
void Point::serialize(Archive& ar, const unsigned int /*version*/)
{
	ar & x;
	ar & y;
}

template<class Archive>
void UPoint::serialize(Archive& ar, const unsigned int /*version*/)
{
	ar & x;
	ar & y;
}

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
