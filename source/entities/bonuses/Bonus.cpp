#include "entities/bonuses/Bonus.h"
#include "components/EventSystem.h"
#include "components/events/AnimationRenderEvents.h"
#include "components/events/CoreLifecycleEvents.h"
#include "components/events/ObjectLifecycleEvents.h"
#include "components/events/StatisticsEvents.h"
#include "components/events/BonusPickupEvents.h"
#include "entities/BaseObjProperty.h"
#include "enums/BonusType.h"
#include "enums/Direction.h"
#include "enums/GameMode.h"
#include "enums/Faction.h"
#include "utils/TimeUtils.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>
#include <ranges>

namespace
{
//NOTE: what used to be a subclass each - a name and the event a pickup emits. Everything else about a
//bonus was already the same code. Captureless, so the emitter stays a plain pointer and the whole
//catalogue can be constexpr
using PickupEmitter = void (*)(EventSystem& events, const std::string& author, Faction faction);

struct BonusRecipe
{
	BonusType type{};
	std::string_view name{};
	PickupEmitter emit{};
};

constexpr std::array s_recipes{
		BonusRecipe{.type = BonusType::Timer, .name = "BonusTimer",
					.emit = [](EventSystem& events, const std::string&, Faction faction)
					{
						//NOTE: a timer freezes the other side, same as a grenade wipes it
						events.EmitEvent(BonusTimerPickupEvent{.target = EnemiesOf(faction)});
					}},
		BonusRecipe{.type = BonusType::Helmet, .name = "BonusHelmet",
					.emit = [](EventSystem& events, const std::string& author, Faction)
					{
						events.EmitEvent(BonusHelmetPickupEvent{.author = author});
					}},
		BonusRecipe{.type = BonusType::Grenade, .name = "BonusGrenade",
					.emit = [](EventSystem& events, const std::string&, Faction faction)
					{
						//NOTE: the one bonus whose effect lands on the other side
						events.EmitEvent(Key(EnemiesOf(faction)), BonusGrenadePickupEvent{});
					}},
		BonusRecipe{.type = BonusType::Tank, .name = "BonusTank",
					.emit = [](EventSystem& events, const std::string& author, Faction)
					{
						events.EmitEvent(BonusTankPickupEvent{.author = author});
					}},
		BonusRecipe{.type = BonusType::Star, .name = "BonusStar",
					.emit = [](EventSystem& events, const std::string& author, Faction)
					{
						events.EmitEvent(Key(author), BonusStarPickupEvent{});
					}},
		BonusRecipe{.type = BonusType::Shovel, .name = "BonusShovel",
					.emit = [](EventSystem& events, const std::string&, Faction faction)
					{
						events.EmitEvent(BonusShovelPickupEvent{.faction = faction});
					}},
		BonusRecipe{.type = BonusType::Caliber, .name = "BonusCaliber",
					.emit = [](EventSystem& events, const std::string& author, Faction)
					{
						events.EmitEvent(Key(author), BonusCaliberPickupEvent{});
					}},
		BonusRecipe{.type = BonusType::Ship, .name = "BonusShip",
					.emit = [](EventSystem& events, const std::string& author, Faction)
					{
						events.EmitEvent(Key(author), BonusShipPickupEvent{});
					}},
};

static_assert(s_recipes.size() == static_cast<std::size_t>(BonusType::lastId) - 1u,
			  "Amount of recipes should be equal to the spawnable BonusType count");

static_assert(
		std::ranges::all_of(
				std::views::iota(std::size_t{0}, s_recipes.size()),
				[](const std::size_t i) { return static_cast<std::size_t>(s_recipes[i].type) == i + 1u; }),
		"Order of recipes should be equal to enum BonusType");

[[nodiscard]] const BonusRecipe& GetRecipe(const BonusType type)
{
	//NOTE: BonusSpawner gates on IsSpawnableBonus before it ever gets here
	return s_recipes[static_cast<std::size_t>(type) - 1u];
}

//NOTE: driven by the clock rather than by a tick - bonuses do not tick on the client, but they do draw
[[nodiscard]] unsigned int SuperRimColor()
{
	using namespace std::chrono;
	constexpr auto period = milliseconds{1200};
	constexpr unsigned int dark{0xB8860Bu};//NOTE: dark goldenrod
	constexpr unsigned int light{0xFFEC8Cu};//NOTE: pale gold

	const auto phase = duration_cast<milliseconds>(TimeUtils::Now().time_since_epoch()) % period;
	const float progress = static_cast<float>(phase.count()) / static_cast<float>(period.count());
	const float wave = 0.5f * (1.f - std::cos(progress * 2.f * std::numbers::pi_v<float>));

	const auto blend = [wave](const unsigned int shift)
	{
		const auto from = static_cast<float>(dark >> shift & 0xFFu);
		const auto to = static_cast<float>(light >> shift & 0xFFu);

		return static_cast<unsigned int>(std::lerp(from, to, wave)) << shift;
	};

	return 0xFF000000u | blend(16u) | blend(8u) | blend(0u);
}
}//namespace

Bonus::Bonus(const ObjRectangle& rect, const std::shared_ptr<EventSystem>& events, const Uuid uuid,
			 const GameMode gameMode, const BonusType bonusType, const bool isSuper)
	: BaseObj{BaseObjProperty{.rect = rect,
							  .health = 1,
							  .uuid = uuid,
							  .name = std::string{GetRecipe(bonusType).name},
							  .faction = Faction::Neutral},
			  kCollision}
	, _gameMode{gameMode}
	, _bonusType{bonusType}
	, _isSuper{isSuper}
	, _events{events}
{
	Subscribe();
}

Bonus::~Bonus() = default;

//NOTE: the one way off the field - picked up, expired or shot, the announcement is the same
void Bonus::Despawn(const DespawnReason reason)
{
	SetIsAlive(false);

	if (IsHost(_gameMode))
	{
		_events->EmitEvent(DespawnedEvent{.who = _name, .uuid = _uuid, .reason = reason});
	}
}

void Bonus::Subscribe()
{
	_subs.push_back(_events->AddListener(this, &Bonus::OnDraw));

	if (!IsAuthority(_gameMode))
	{
		SubscribeAsClient();
	}
}

void Bonus::OnDraw(const DrawEvent&) const { Draw(); }

void Bonus::SubscribeAsClient()
{
	_subs.push_back(_events->AddListener(Key(_uuid), this, &Bonus::OnDespawned));
}

void Bonus::OnDespawned(const DespawnedEvent&) { SetIsAlive(false); }

void Bonus::Draw() const
{
	_events->EmitEvent(DrawObjEvent{.rect = _rect,
									.dir = Direction::UP,
									.name = _name,
									.rimColor = _isSuper ? SuperRimColor() : 0u});
}

bool Bonus::GetIsSuper() const { return _isSuper; }

void Bonus::Expire()
{
	if (!GetIsAlive())
	{
		return;
	}

	_events->EmitEvent(StatisticsBonusExpiredEvent{});

	Despawn(DespawnReason::Expired);
}

void Bonus::TakeDamage(const unsigned int damage, const std::string& author, Faction faction)
{
	if (!GetIsAlive())
	{
		return;
	}

	BaseObj::TakeDamage(damage, author, faction);

	if (!GetIsAlive())
	{
		Despawn(DespawnReason::Destroyed);
	}
}

void Bonus::EmitDamageStatistics(const std::string& author, Faction faction)
{
	_events->EmitEvent(StatisticsBonusDestroyedEvent{.author = author, .faction = faction});
}

void Bonus::PickUpBonus(const std::string& author, Faction faction)
{
	if (GetIsAlive())
	{
		_events->EmitEvent(StatisticsBonusPickupEvent{.author = author, .faction = faction});

		const PickupEmitter emit = GetRecipe(_bonusType).emit;
		emit(*_events, author, faction);

		if (_isSuper)
		{
			//NOTE: a super bonus is simply its own effect twice - timed ones stack their duration,
			//stepped ones (a tier, a life) advance one more step
			emit(*_events, author, faction);
		}

		Despawn(DespawnReason::PickedUp);
	}
}
