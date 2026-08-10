#pragma once

#include <type_traits>

namespace tags
{
struct Passable {};

struct Impassable {};

struct Destructible {};

struct Indestructible {};

struct Penetrable {};

struct Impenetrable {};
}

struct CollisionTags
{
	bool passable;
	bool destructible;
	bool penetrable;

	template<typename... Tags>
	constexpr explicit CollisionTags(Tags...)
		: passable{(0 + ... + std::is_same_v<Tags, tags::Passable>) == 1}
		, destructible{(0 + ... + std::is_same_v<Tags, tags::Destructible>) == 1}
		, penetrable{(0 + ... + std::is_same_v<Tags, tags::Penetrable>) == 1}
	{
		static_assert((0 + ... + (std::is_same_v<Tags, tags::Passable> || std::is_same_v<Tags, tags::Impassable>)) == 1,
					  "CollisionTags requires exactly one of tags::Passable or tags::Impassable");
		static_assert(
				(0 + ... + (std::is_same_v<Tags, tags::Destructible> || std::is_same_v<Tags, tags::Indestructible>)) ==
				1,
				"CollisionTags requires exactly one of tags::Destructible or tags::Indestructible");
		static_assert(
				(0 + ... + (std::is_same_v<Tags, tags::Penetrable> || std::is_same_v<Tags, tags::Impenetrable>)) == 1,
				"CollisionTags requires exactly one of tags::Penetrable or tags::Impenetrable");
	}
};
