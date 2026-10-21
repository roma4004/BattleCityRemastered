#pragma once

#include "enums/Author.h"
#include "enums/BonusType.h"
#include "enums/Faction.h"
#include <variant>

//NOTE: an effect outlives its bonus, so it is keyed by what it sits on - a seat or a faction
using EffectTarget = std::variant<Author, Faction>;

//NOTE: a timer freezes the taker's opponents - the other team, or in a free-for-all every tank but the taker
[[nodiscard]] inline bool FreezesSeat(const BonusType type, const EffectTarget& target, const Author spared,
									  const Faction side, const Author seat)
{
	return type == BonusType::Timer && target == EffectTarget{side} && spared != seat;
}
