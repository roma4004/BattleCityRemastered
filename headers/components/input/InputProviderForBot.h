#pragma once

#include "interfaces/IInputProvider.h"
#include "utils/Timer.h"
#include "utils/Uuid.h"
#include <array>
#include <chrono>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

enum class Direction : char8_t;
struct ObjRectangle;
class BaseObj;
class GameConfig;
class LineOfSight;
class Tank;

class InputProviderForBot final : public IInputProvider
{
	//NOTE: found once a frame and read by both halves - the dodge and the intercept are one decision
	struct BulletThreat final
	{
		std::shared_ptr<BaseObj> bullet{nullptr};
		//NOTE: seconds, not a distance - a higher tier's shell flies faster
		double timeToImpact{};
		//NOTE: where it is going, which is the axis a dodge has to leave
		Direction flying{};
		//NOTE: it is coming straight at us, rather than crossing the lane we happen to stand in
		bool isHeadOn{};
	};

	const std::vector<std::shared_ptr<BaseObj>>& _allObjects;
	const GameConfig& _gameConfig;

	//NOTE: milliseconds, not seconds - the roll lands in the unit of its bounds
	static constexpr std::chrono::milliseconds kMinTurnDelay{std::chrono::seconds{1}};
	static constexpr std::chrono::milliseconds kMaxTurnDelay{std::chrono::seconds{5}};
	//NOTE: the least time between two turns of the same hull - it paces the bot that keeps hitting things
	static constexpr std::chrono::milliseconds kTurnFloor{300};
	Timer _randomChangeDirTimer{};
	Timer _turnFloor{};

	//NOTE: how long an opponent stands in the sights before the bot acts - read by the side it stands on
	struct NoticeBand
	{
		std::chrono::milliseconds from{};
		std::chrono::milliseconds to{};
	};

	static constexpr std::array kNoticeLadder{NoticeBand{.from = std::chrono::milliseconds{500},
														.to = std::chrono::seconds{1}},
											  NoticeBand{.from = std::chrono::seconds{1},
														 .to = std::chrono::seconds{2}},
											  NoticeBand{.from = std::chrono::seconds{2},
														 .to = std::chrono::seconds{5}},
											  NoticeBand{.from = std::chrono::seconds{5},
														 .to = std::chrono::seconds{10}}};
	static constexpr std::size_t kNoticeAheadRung{1u};
	static constexpr std::size_t kNoticeFlankRung{2u};
	static constexpr std::size_t kNoticeBehindRung{3u};

	//NOTE: one per side of the world, not per side of the hull - a turn must not restart the delay
	struct SideNotice
	{
		Uuid target{};
		Timer delay{};
		//NOTE: under fire the wait re-rolls a rung faster - a shot at a back is noticed, driving behind it is not
		bool isUnderFire{};
	};

	std::array<SideNotice, 4u> _notices{};

	//NOTE: the drivable pass, built at most once per TurnOntoNearestSeen and shared by all four sides
	std::unique_ptr<LineOfSight> _driveLineOfSight{};

	//NOTE: started by a refusal to fire at a wall, and the refusal stands while it ticks
	Timer _obstacleShootCooldown{};

	//NOTE: found in ChooseDirection and read again in ShouldShoot, which run in that order
	BulletThreat _threat{};
	//NOTE: the hull is held for the shot - answered anew every tick, a target that drove off frees it
	bool _isLinedUpForShot{};

	//NOTE: below this our shell cannot meet theirs in time - they close at the sum of speeds, ours a frame late
	static constexpr double kInterceptWindowSeconds{0.12};

	[[nodiscard]] BulletThreat FindBulletThreat(const Tank& self) const;
	[[nodiscard]] bool CanIntercept() const;
	//NOTE: what the bullet would hit before it reaches us - a shot behind steel is the steel's business
	[[nodiscard]] bool IsShotStoppedOnTheWay(const ObjRectangle& corridor, const BaseObj& bullet,
											 const Tank& self) const;

	//NOTE: across the bullet's path, and only to a side the whole hull can clear - else it shakes in a narrow passage
	[[nodiscard]] std::optional<Direction> SideOutOfLane(const Tank& self, const BulletThreat& threat,
														 double deltaTime) const;

	[[nodiscard]] bool ChangeDirIfSeenBonus(Tank& self, Direction dir,
											const std::vector<std::shared_ptr<BaseObj>>& sideObstacle);
	[[nodiscard]] bool ChangeDirIfSeenOpponent(Tank& self, Direction dir,
											   const std::vector<std::shared_ptr<BaseObj>>& sideObstacle);
	//NOTE: what makes a side worth turning to - the two lookups differ by this and nothing else
	using SightTrigger = bool (InputProviderForBot::*)(Tank&, Direction,
													  const std::vector<std::shared_ptr<BaseObj>>&);

	[[nodiscard]] std::shared_ptr<BaseObj> Lookup(Tank& self, LineOfSight& lineOfSight, Direction& dir,
												  SightTrigger trigger);

	[[nodiscard]] bool IsCenteredOn(const Tank& self, Direction dir, const BaseObj& target) const;
	//NOTE: arms the delay on the first sighting of that target and answers whether it has run out
	[[nodiscard]] bool HasNoticed(const Tank& self, Direction side, const BaseObj& target);
	[[nodiscard]] static NoticeBand NoticeBandFor(Direction heading, Direction side, bool isUnderFire);

	[[nodiscard]] bool CanDriveToBonus(const Tank& self, Direction dir);

	[[nodiscard]] std::shared_ptr<BaseObj> TurnOntoNearestSeen(Tank& self);

	//NOTE: after a turn of our own - else the random one fires on the next frame and undoes it
	void PostponeRandomTurn();
	[[nodiscard]] std::optional<Direction> PickRandomDirection(const Tank& self, double deltaTime,
															   bool excludeCurrentDirection = false);

	[[nodiscard]] bool ShouldShootOpponent(const Tank& self, const std::shared_ptr<BaseObj>& obj);
	[[nodiscard]] bool RollShootObstacle(const std::shared_ptr<BaseObj>& obj);

public:
	InputProviderForBot(const std::vector<std::shared_ptr<BaseObj>>& allObjects, const GameConfig& gameConfig);

	~InputProviderForBot() override;

	[[nodiscard]] std::optional<Direction> ChooseDirection(Tank& self, double deltaTime) override;
	[[nodiscard]] std::optional<Direction> ReviseWhenMoveBlocked(Tank& self, double deltaTime) override;
	[[nodiscard]] bool ShouldShoot(Tank& self) override;
};
