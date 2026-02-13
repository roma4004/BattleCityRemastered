#pragma once

template<class Archive>
void ServerData::serialize(Archive& ar, const unsigned int /*version*/)
{
	ar & eventName;
}
