#include "network/commands/BonusSpawn.h"

namespace network::commands
{
BonusSpawn::BonusSpawn(const FPoint pos, const BonusType bonusType, const Uuid uuid)
	: _pos{pos}
	, _bonusType{bonusType}
	, _uuid{uuid} {}

CommandType BonusSpawn::GetType() const noexcept { return _type; }

FPoint BonusSpawn::GetPos() const noexcept { return _pos; }

BonusType BonusSpawn::GetBonusType() const noexcept { return _bonusType; }

Uuid BonusSpawn::GetUuid() const noexcept { return _uuid; }

const char* BonusSpawn::GetClassNameW() const noexcept { return "BonusSpawn"; }
}//namespace network::commands
