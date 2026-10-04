#include "components/MapLoader.h"
#include "components/WorldGeometry.h"
#include "enums/BonusType.h"
#include "enums/TankModel.h"
#include "utils/TextUtils.h"
#include <algorithm>
#include <charconv>
#include <cstddef>
#include <expected>
#include <fstream>
#include <limits>
#include <map>
#include <optional>
#include <ranges>
#include <sstream>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

namespace
{
constexpr char kCommentPrefix{'#'};
//NOTE: the digits line up with ObstacleType by construction, so the legend in the map file is the
//enum order - keep them in step when a new obstacle appears
constexpr char kFirstSymbol{'0'};
constexpr std::string_view kBlanks{" \t"};
//NOTE: "20 enemies: 6 basic, 2 fast" - a grid row holds no blank, so the line never reads as one
constexpr std::string_view kEnemiesKey{"enemies:"};
//NOTE: the enemy counter is an unsigned short
constexpr std::size_t kMaxListedEnemies{std::numeric_limits<unsigned short>::max()};

[[nodiscard]] bool IsSkippable(const std::string_view line)
{
	const std::size_t firstVisible{line.find_first_not_of(kBlanks)};

	return firstVisible == std::string_view::npos || line[firstVisible] == kCommentPrefix;
}

//NOTE: bonuses are letters, obstacles digits - so the grid still reads as a picture, and a new obstacle
//cannot collide with a bonus by sliding the legend
[[nodiscard]] BonusType BonusOfSymbol(const char symbol)
{
	switch (symbol)
	{
		case 't':
			return BonusType::Timer;
		case 'h':
			return BonusType::Helmet;
		case 'g':
			return BonusType::Grenade;
		case 'l':
			return BonusType::Tank;
		case 's':
			return BonusType::Star;
		case 'v':
			return BonusType::Shovel;
		case 'c':
			return BonusType::Caliber;
		case 'b':
			return BonusType::Ship;
		default:
			return BonusType::None;
	}
}

//NOTE: only what stops a tank - bush and ice are ground it drives over and spawns on, so counting them
//would refuse a perfectly playable map. WorldQuery draws the same line, and the two have to agree
[[nodiscard]] std::vector<bool> BlockedCells(const MapData& map)
{
	std::vector<bool> blocked(map.cols * map.rows, false);

	for (const auto [row, col]: std::views::cartesian_product(std::views::iota(std::size_t{}, map.rows),
															  std::views::iota(std::size_t{}, map.cols)))
	{
		const ObstacleType type{map.At(col, row)};
		if (!IsSpawnableObstacle(type) || IsDrivableObstacle(type))
		{
			continue;
		}

		//NOTE: the eagle is one character but four cells across, and its own body reads as free ground
		const auto span{static_cast<std::size_t>(ObstacleCellSpan(type))};
		for (const auto [covered, across]:
			 std::views::cartesian_product(std::views::iota(row, std::min(row + span, map.rows)),
										   std::views::iota(col, std::min(col + span, map.cols))))
		{
			blocked[covered * map.cols + across] = true;
		}
	}

	return blocked;
}

//NOTE: what TankSpawner asks of the world - does a tank-sized square fit anywhere in this strip. Without
//one the spawner waits for a spot that never comes
[[nodiscard]] bool HasTankWideOpening(const MapData& map, const std::vector<bool>& blocked,
									  const std::size_t firstRow)
{
	constexpr std::size_t span{WorldGeometry::kTankCellSpan};

	auto isFree = [&map, &blocked, firstRow](const std::size_t col)
	{
		return std::ranges::all_of(std::views::iota(firstRow, firstRow + span), [&](const std::size_t row)
		{
			return std::ranges::none_of(std::views::iota(col, col + span), [&](const std::size_t across)
			{
				return blocked[row * map.cols + across];
			});
		});
	};

	return std::ranges::any_of(std::views::iota(std::size_t{}, map.cols - span + 1u), isFree);
}

[[nodiscard]] std::string_view Trimmed(const std::string_view text)
{
	const std::size_t first{text.find_first_not_of(kBlanks)};
	if (first == std::string_view::npos)
	{
		return {};
	}

	return text.substr(first, text.find_last_not_of(kBlanks) - first + 1u);
}

[[nodiscard]] bool IsEnemiesLine(const std::string_view line)
{
	const std::string_view text{Trimmed(line)};
	const std::size_t digits{std::min(text.find_first_not_of("0123456789"), text.size())};

	return digits > 0u && Trimmed(text.substr(digits)).starts_with(kEnemiesKey);
}

//NOTE: any case, so "Armor" the way the log spells it reads too; the player's model is no enemy
[[nodiscard]] std::optional<TankModel> EnemyModelNamed(const std::string_view name)
{
	const auto models{std::views::iota(kFirstTankModelId, kLastEnemyModelId + 1)
					  | std::views::transform([](const int id) { return static_cast<TankModel>(id); })};
	const auto found{std::ranges::find_if(models, [name](const TankModel model)
	{
		return std::ranges::equal(name, ToString(model), {}, TextUtils::ToLower, TextUtils::ToLower);
	})};
	if (found == models.end())
	{
		return std::nullopt;
	}

	return *found;
}

//NOTE: "fast" is one enemy and "4 armor" four in a row, in the order the list gives them
[[nodiscard]] std::expected<std::vector<TankModel>, std::string> ParseLineup(const std::string_view list)
{
	std::vector<TankModel> lineup;
	for (const auto entry: list | std::views::split(','))
	{
		std::string_view item{Trimmed(std::string_view{entry.begin(), entry.end()})};
		std::size_t count{1u};
		const auto [end, error]{std::from_chars(item.data(), item.data() + item.size(), count)};
		if (error == std::errc{})
		{
			item = Trimmed(item.substr(static_cast<std::size_t>(end - item.data())));
		}

		if (error == std::errc::result_out_of_range || count > kMaxListedEnemies - lineup.size())
		{
			return std::unexpected("the enemy list holds more than " + std::to_string(kMaxListedEnemies)
								   + " tanks");
		}

		if (count == 0u)
		{
			return std::unexpected(std::string{"a count of 0 in the enemy list - leave the entry out"});
		}

		if (item.empty())
		{
			return std::unexpected(std::string{"an entry of the enemy list names no model"});
		}

		const std::optional<TankModel> model{EnemyModelNamed(item)};
		if (!model)
		{
			return std::unexpected("unknown enemy '" + std::string{item} + "' - see the legend");
		}

		lineup.insert(lineup.end(), count, *model);
	}

	return lineup;
}

//NOTE: the count, then the models of the first enemies - the list may be shorter or left out, the rest are rolled
[[nodiscard]] std::expected<void, std::string> ReadEnemies(const std::string_view line, MapData& map)
{
	const std::string_view text{Trimmed(line)};
	std::size_t count{};
	const auto [end, error]{std::from_chars(text.data(), text.data() + text.size(), count)};
	if (error == std::errc::result_out_of_range || count > kMaxListedEnemies)
	{
		return std::unexpected("a level holds at most " + std::to_string(kMaxListedEnemies) + " enemies");
	}

	if (count == 0u)
	{
		return std::unexpected(std::string{"a level of 0 enemies is never won - leave the line out for 20"});
	}

	const std::string_view rest{Trimmed(text.substr(static_cast<std::size_t>(end - text.data())))};
	auto lineup{ParseLineup(rest.substr(kEnemiesKey.size()))};
	if (!lineup)
	{
		return std::unexpected(std::move(lineup).error());
	}

	if (lineup->size() > count)
	{
		return std::unexpected("the list names " + std::to_string(lineup->size()) + " enemies, and the level has "
							   + std::to_string(count));
	}

	map.enemyCount = count;
	map.enemyLineup = std::move(*lineup);

	return {};
}

[[nodiscard]] bool IsKnownSymbol(const char symbol)
{
	if (IsSpawnableBonus(BonusOfSymbol(symbol)))
	{
		return true;
	}

	if (symbol < kFirstSymbol)
	{
		return false;
	}

	const auto type{static_cast<ObstacleType>(symbol - kFirstSymbol)};

	return type == ObstacleType::None || IsSpawnableObstacle(type);
}
}// namespace

std::expected<MapData, MapError> MapLoader::LoadFromFile(const std::filesystem::path& path)
{
	//NOTE: a map file never changes while the game runs, so every load after the first skips the disk
	static std::map<std::filesystem::path, MapData> parsed;
	if (const auto cached{parsed.find(path)}; cached != parsed.end())
	{
		return cached->second;
	}

	const std::ifstream file{path};
	if (!file)
	{
		return std::unexpected(MapError{.path = path, .reason = "cannot open the map file"});
	}

	std::ostringstream contents;
	contents << file.rdbuf();

	//NOTE: only a good parse is remembered - a broken path goes back to the disk and complains again
	auto loaded{Parse(contents.str(), path)};
	if (!loaded)
	{
		return std::unexpected(std::move(loaded).error());
	}

	if (auto playable{Validate(*loaded, path)};
		!playable)
	{
		return std::unexpected(std::move(playable).error());
	}

	return parsed.emplace(path, std::move(*loaded)).first->second;
}

std::expected<MapData, MapError> MapLoader::Parse(const std::string_view text, std::filesystem::path path)
{
	MapData map{};
	std::size_t lineNumber{};

	for (std::size_t pos = 0u; pos <= text.size();)
	{
		const std::size_t lineEnd{std::min(text.find('\n', pos), text.size())};
		std::string_view line{text.substr(pos, lineEnd - pos)};
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

		if (IsEnemiesLine(line))
		{
			if (map.enemyCount)
			{
				return std::unexpected(MapError{.path = std::move(path),
												.reason = "the enemies are counted twice",
												.line = lineNumber});
			}

			if (auto read{ReadEnemies(line, map)}; !read)
			{
				return std::unexpected(MapError{.path = std::move(path),
												.reason = std::move(read).error(),
												.line = lineNumber});
			}

			continue;
		}

		if (map.cols == 0u)
		{
			map.cols = line.size();
		}
		else if (line.size() != map.cols)
		{
			return std::unexpected(MapError{.path = std::move(path),
											.reason = "row is " + std::to_string(line.size()) +
													  " cells wide, but the map is "
													  + std::to_string(map.cols),
											.line = lineNumber});
		}

		for (const char symbol: line)
		{
			if (!IsKnownSymbol(symbol))
			{
				return std::unexpected(MapError{.path = std::move(path),
												.reason = std::string{"unknown symbol '"} + symbol +
														  "' - see the legend",
												.line = lineNumber});
			}

			//NOTE: the cell under a bonus is free ground - the bonus lies on it, it is not the terrain
			if (const BonusType bonus{BonusOfSymbol(symbol)}; IsSpawnableBonus(bonus))
			{
				map.bonuses.push_back(BonusPlacement{.col = map.cells.size() % map.cols,
													 .row = map.rows,
													 .type = bonus});
				map.cells.push_back(ObstacleType::None);

				continue;
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

std::expected<void, MapError> MapLoader::Validate(const MapData& map, std::filesystem::path path)
{
	constexpr std::size_t span{WorldGeometry::kTankCellSpan};
	if (map.cols < span || map.rows < span)
	{
		return std::unexpected(MapError{.path = std::move(path),
										.reason = "the map is " + std::to_string(map.cols) + " x " +
												  std::to_string(map.rows) + " cells, and a tank alone is "
												  + std::to_string(span) + " across"});
	}

	if (const auto eagles{std::ranges::count(map.cells, ObstacleType::Eagle)};
		eagles != 1)
	{
		return std::unexpected(MapError{.path = std::move(path),
										.reason = "the map has " + std::to_string(eagles) +
												  " eagles, and a match needs exactly one"});
	}

	const std::vector<bool> blocked{BlockedCells(map)};
	auto noOpening = [&path](const std::string_view edge)
	{
		return std::unexpected(MapError{.path = path,
										.reason = "nothing fits through the " + std::string{edge} +
												  " edge - a tank has nowhere to spawn"});
	};

	if (!HasTankWideOpening(map, blocked, 0u))
	{
		return noOpening("top");
	}

	if (!HasTankWideOpening(map, blocked, map.rows - span))
	{
		return noOpening("bottom");
	}

	return {};
}
