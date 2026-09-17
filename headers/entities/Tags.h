#pragma once

#include "enums/Terrain.h"
#include <type_traits>

namespace tags
{
struct Passable {};

struct Impassable {};

struct Destructible {};

struct Indestructible {};

struct Penetrable {};

struct Impenetrable {};

struct NoTerrain {};

struct Water {};

struct Ice {};

struct Bush {};
}// namespace tags

struct CollisionTags
{
	bool passable;
	bool destructible;
	bool penetrable;
	Terrain terrain;

	template<typename... Tags>
	constexpr explicit CollisionTags(Tags...)
		: passable{(0 + ... + std::is_same_v<Tags, tags::Passable>) == 1}
		, destructible{(0 + ... + std::is_same_v<Tags, tags::Destructible>) == 1}
		, penetrable{(0 + ... + std::is_same_v<Tags, tags::Penetrable>) == 1}
		, terrain{TerrainOf<Tags...>()}
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
		static_assert((0 + ... + (std::is_same_v<Tags, tags::NoTerrain> || std::is_same_v<Tags, tags::Water>
								  || std::is_same_v<Tags, tags::Ice> || std::is_same_v<Tags, tags::Bush>)) == 1,
					  "CollisionTags requires exactly one of tags::NoTerrain, tags::Water, tags::Ice or tags::Bush");
	}

private:
	template<typename... Tags>
	[[nodiscard]] static constexpr Terrain TerrainOf()
	{
		if constexpr ((0 + ... + std::is_same_v<Tags, tags::Water>) == 1)
		{
			return Terrain::Water;
		}

		if constexpr ((0 + ... + std::is_same_v<Tags, tags::Ice>) == 1)
		{
			return Terrain::Ice;
		}

		if constexpr ((0 + ... + std::is_same_v<Tags, tags::Bush>) == 1)
		{
			return Terrain::Bush;
		}

		return Terrain::None;
	}
};
