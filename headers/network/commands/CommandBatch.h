#pragma once

#include "Command.h"
#include <boost/serialization/access.hpp>
#include <boost/serialization/export.hpp>
#include <boost/serialization/shared_ptr.hpp>
#include <boost/serialization/vector.hpp>

class CommandBatch : public Command
{
	friend class boost::serialization::access;

	std::vector<std::shared_ptr<Command>> _commands;

public:
	CommandBatch();
	~CommandBatch() override = default;

	void AddCommand(const std::shared_ptr<Command>& command);
	[[nodiscard]] const std::vector<std::shared_ptr<Command>>& GetCommands() const noexcept;

	[[nodiscard]] const char* GetClassNameW() const noexcept override;
	[[nodiscard]] size_t GetSize() const noexcept;
	[[nodiscard]] bool IsEmpty() const noexcept;

	template<class Archive>
	void serialize(Archive& ar, const unsigned int /*version*/);
};

template<class Archive>
void CommandBatch::serialize(Archive& ar, const unsigned int)
{
	ar & boost::serialization::base_object<Command>(*this);
	ar & _commands;
}

BOOST_CLASS_EXPORT_KEY(CommandBatch);
