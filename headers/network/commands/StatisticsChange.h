#pragma once

#include "enums/CommandType.h"
#include "enums/StatisticsType.h"
#include <ser20/types/common.hpp>
#include <ser20/types/string.hpp>
#include <string>

namespace network::commands
{
class StatisticsChange
{
	CommandType _type{CommandType::STATISTICS_CHANGE};
	StatisticsType _statisticsType{};
	std::string _author{};
	std::string _fraction{};

public:
	//for deserialization
	StatisticsChange() = default;

	//for serialization
	StatisticsChange(StatisticsType type, std::string author, std::string fraction);

	[[nodiscard]] CommandType GetType() const noexcept;
	[[nodiscard]] StatisticsType GetStatisticsType() const noexcept;
	[[nodiscard]] std::string GetAuthor() const noexcept;
	[[nodiscard]] std::string GetFraction() const noexcept;
	[[nodiscard]] const char* GetClassNameW() const noexcept;

	template<class Archive>
	void serialize(Archive& ar, unsigned int /*version*/);
};

template<class Archive>
void StatisticsChange::serialize(Archive& ar, const unsigned int)
{
	ar & _type;
	ar & _statisticsType;
	ar & _author;
	ar & _fraction;
}
}//namespace network::commands
