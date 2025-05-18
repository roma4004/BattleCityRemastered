#include "../../../headers/network/commands/Command.h"

Command::Command(const CommandType type): _type(type) {}

CommandType Command::GetType() const { return _type; }
