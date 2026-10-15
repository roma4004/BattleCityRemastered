#pragma once

#include "enums/Author.h"
#include "enums/Faction.h"
#include <variant>

//NOTE: an effect outlives its bonus, so it is keyed by what it sits on - a seat or a faction
using EffectTarget = std::variant<Author, Faction>;
