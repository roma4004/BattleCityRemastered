#pragma once

#include "Point.h"
#include "enums/Direction.h"
#include <boost/uuid/uuid.hpp>
#include <string>

using buuid = boost::uuids::uuid;

struct ServerSendPosEvent
{
	std::string who;
	FPoint pos;
	Direction dir;
	buuid uuid;
};

struct ServerSendShotEvent
{
	std::string who;
	Direction dir;
	buuid bulletUuid;
};

struct ServerSendHealthEvent
{
	std::string who;
	int health;
	buuid uuid;
};

struct ServerSendOnTankOnOffEvent
{
	buuid uuid;
	bool isEnable;
	std::string name;
};

struct ServerSendBonusHelmetPickupEvent
{
	std::string name;
	bool isActive;
};

struct ClientReceivedPosEvent
{
	FPoint pos;
	Direction dir;
};

struct ClientReceivedShotEvent
{
	Direction dir;
	buuid bulletUuid;
};

struct ClientReceivedOnTankOnOffEvent
{
	bool isEnable;
};

struct ClientReceivedBonusHelmetPickupEvent
{
	bool isEnable;
};

struct ClientReceivedHealthEvent
{
	int health;
};
