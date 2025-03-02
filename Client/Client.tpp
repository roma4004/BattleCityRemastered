#pragma once

template<class Archive>
void Data::serialize(Archive& ar, const unsigned int version)
{
	ar & health;
	ar & id;
	ar & typeId;
	ar & objectName;
	ar & eventName;
	ar & names;
	ar & newPos;
	ar & direction;
}
