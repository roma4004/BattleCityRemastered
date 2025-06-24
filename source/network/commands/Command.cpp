#include "network/commands/Command.h"

Command::Command(const CommandType type) : _type(type) {}

CommandType Command::GetType() const { return _type; }

const char* Command::GetCommandType() const
{
	switch (_type)
	{
		case COMMAND_BATCH:
			return "COMMAND_BATCH";
		case POSITION_CHANGE:
			return "POSITION_CHANGE";
		case TANK_SHOT:
			return "TANK_SHOT";
		case HEALTH_CHANGE:
			return "HEALTH_CHANGE";
		case DISPOSE:
			return "DISPOSE";
		case STATISTICS_CHANGE:
			return "STATISTICS_CHANGE";
		case KEY_STATE_CHANGE:
			return "KEY_STATE_CHANGE";
		case FORTRESS_CHANGE:
			return "FORTRESS_CHANGE";
		case BONUS_SPAWN:
			return "BONUS_SPAWN";
		case BONUS_DESPAWN:
			return "BONUS_DESPAWN";
		case RESPAWN_TANK:
			return "RESPAWN_TANK";
		case OBSTACLE_SPAWN:
			return "OBSTACLE_SPAWN";
		default:
			return "UNKNOWN_COMMAND";
	}
}
