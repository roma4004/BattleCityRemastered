#include "components/MapLoader.h"
#include <fstream>
#include <sstream>

namespace
{
constexpr char kCommentPrefix{'#'};
constexpr char kFirstSymbol{'0'};

[[nodiscard]] bool IsSkippable(const std::string_view line)
{
	const std::size_t firstVisible = line.find_first_not_of(" \t");

	return firstVisible == std::string_view::npos || line[firstVisible] == kCommentPrefix;
}

//NOTE: the digits line up with ObstacleType by construction, so the legend in the map file is the
//enum order - keep them in step when a new obstacle appears
[[nodiscard]] bool IsKnownSymbol(const char symbol)
{
	if (symbol < kFirstSymbol)
	{
		return false;
	}

	const auto type = static_cast<ObstacleType>(symbol - kFirstSymbol);

	return type == ObstacleType::None || IsSpawnableObstacle(type);
}
}

std::expected<MapData, MapError> MapLoader::LoadFromFile(const std::string& path)
{
	std::ifstream file{path};
	if (!file)
	{
		return std::unexpected(MapError{.path = path, .reason = "cannot open the map file"});
	}

	std::ostringstream contents;
	contents << file.rdbuf();

	return Parse(contents.str(), path);
}

std::expected<MapData, MapError> MapLoader::Parse(const std::string_view text, std::string path)
{
	MapData map{};
	std::size_t lineNumber{0u};

	for (std::size_t pos = 0u; pos <= text.size();)
	{
		const std::size_t lineEnd = std::min(text.find('\n', pos), text.size());
		std::string_view line = text.substr(pos, lineEnd - pos);
		pos = lineEnd + 1u;
		++lineNumber;

		//NOTE: a CRLF file would otherwise end every row with an unknown symbol
		if (line.ends_with('\r'))
		{
			line.remove_suffix(1u);
		}

		if (IsSkippable(line))
		{
			continue;
		}

		if (map.cols == 0u)
		{
			map.cols = line.size();
		}
		else if (line.size() != map.cols)
		{
			return std::unexpected(MapError{.path = std::move(path),
											.reason = "row is " + std::to_string(line.size()) + " cells wide, but the map is "
													  + std::to_string(map.cols),
											.line = lineNumber});
		}

		for (const char symbol: line)
		{
			if (!IsKnownSymbol(symbol))
			{
				return std::unexpected(MapError{.path = std::move(path),
												.reason = std::string{"unknown symbol '"} + symbol + "' - see the legend",
												.line = lineNumber});
			}

			map.cells.push_back(static_cast<ObstacleType>(symbol - kFirstSymbol));
		}

		++map.rows;
	}

	if (map.rows == 0u)
	{
		return std::unexpected(MapError{.path = std::move(path), .reason = "no grid in the file, only comments"});
	}

	return map;
}
