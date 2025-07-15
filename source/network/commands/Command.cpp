#include "network/commands/Command.h"

Command::Command(const CommandType type) : _type(type) {}

CommandType Command::GetType() const noexcept { return _type; }

const char* Command::GetCommandType() const noexcept
{
	switch (_type)
	{
		case CommandType::COMMAND_BATCH:
			return "COMMAND_BATCH";
		case CommandType::POSITION_CHANGE:
			return "POSITION_CHANGE";
		case CommandType::TANK_SHOT:
			return "TANK_SHOT";
		case CommandType::HEALTH_CHANGE:
			return "HEALTH_CHANGE";
		case CommandType::DISPOSE:
			return "DISPOSE";
		case CommandType::STATISTICS_CHANGE:
			return "STATISTICS_CHANGE";
		case CommandType::KEY_STATE_CHANGE:
			return "KEY_STATE_CHANGE";
		case CommandType::FORTRESS_CHANGE:
			return "FORTRESS_CHANGE";
		case CommandType::BONUS_SPAWN:
			return "BONUS_SPAWN";
		case CommandType::BONUS_DESPAWN:
			return "BONUS_DESPAWN";
		case CommandType::RESPAWN_TANK:
			return "RESPAWN_TANK";
		case CommandType::OBSTACLE_SPAWN:
			return "OBSTACLE_SPAWN";
		default:
			return "UNKNOWN_COMMAND";
	}
}
