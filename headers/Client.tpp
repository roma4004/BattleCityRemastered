#pragma once

template<class Archive>
void ClientData::serialize(Archive& ar, const unsigned int /*version*/)
{
	ar & health;
	ar & id;
	ar & typeId;
	ar & objectName;
	ar & eventName;
	ar & fraction;
	ar & names;
	ar & newPos;
	ar & direction;
}
