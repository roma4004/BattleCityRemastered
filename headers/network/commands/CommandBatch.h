#pragma once

#include "AnyCommand.h"
#include <ser20/types/vector.hpp>
#include <string>
#include <vector>

namespace network::commands
{
class CommandBatch
{
	std::vector<AnyCommand> _commands;

public:
	CommandBatch() = default;

	void AddCommand(AnyCommand command);
	[[nodiscard]] const std::vector<AnyCommand>& GetCommands() const noexcept;

	[[nodiscard]] size_t GetSize() const noexcept;
	[[nodiscard]] bool IsEmpty() const noexcept;
	[[nodiscard]] std::string GetClassNamesW() const;

	template<class Archive>
	void serialize(Archive& ar, unsigned int /*version*/);
};

template<class Archive>
void CommandBatch::serialize(Archive& ar, const unsigned int)
{
	ar & _commands;
}
}//namespace network::commands
