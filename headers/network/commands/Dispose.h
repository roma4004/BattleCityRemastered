#pragma once

#include <string>
#include <boost/serialization/base_object.hpp>
#include <boost/serialization/export.hpp>

#include "../commands/Command.h"

class Dispose : public Command
{
	friend class boost::serialization::access;

	std::string _who{};
	int _id{};

public:
	//for deserialization
	Dispose();

	//for serialization
	Dispose(const std::string& who, int id);

	~Dispose() override = default;

	const std::string& GetWho() const;
	int GetId() const;

	template<class Archive>
	void serialize(Archive& ar, const unsigned int /*version*/)
	{
		ar & boost::serialization::base_object<Command>(*this);
		ar & _who;
		ar & _id;
	}

	const char* GetClassNameW() const override;
};

BOOST_CLASS_EXPORT_KEY(Dispose);
