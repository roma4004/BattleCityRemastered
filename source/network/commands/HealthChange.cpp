#include "network/commands/HealthChange.h"

namespace network::commands
{
HealthChange::HealthChange(std::string who, const int health, const Uuid uuid)
	: _who{std::move(who)}
	, _health{health}
	, _uuid{uuid} {}

CommandType HealthChange::GetType() const noexcept { return _type; }

std::string HealthChange::GetWho() const noexcept { return _who; }

int HealthChange::GetHealth() const noexcept { return _health; }

Uuid HealthChange::GetUuid() const noexcept { return _uuid; }

const char* HealthChange::GetClassNameW() const noexcept { return "HealthChange"; }
}//namespace network::commands
