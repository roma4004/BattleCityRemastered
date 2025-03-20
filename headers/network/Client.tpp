#pragma once

template<class Archive>
void ClientData::serialize(Archive& ar, const unsigned int /*version*/)
{
	ar & health;
	ar & respawnResource;
	ar & id;
	ar & type;
	ar & who;
	ar & eventType;
	ar & eventName;
	ar & fraction;
	ar & names;
	ar & pos;
	ar & dir;
}
