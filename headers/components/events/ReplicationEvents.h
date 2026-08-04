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

// Payload for the keyed "ClientReceived_Pos" event - the moving object's own uuid is the
// KeyedEvent dispatch key (see EventSystem.h's Key()/AddListener overloads), not a payload field.
struct ClientReceivedPosEvent
{
	FPoint pos;
	Direction dir;
};

// Payload for the keyed "ClientReceived_Shot" event, dispatched by shooting tank NAME (no
// tank-uuid travels on the wire for this command) - bulletUuid identifies the new bullet to
// create client-side, distinct from the key.
struct ClientReceivedShotEvent
{
	Direction dir;
	buuid bulletUuid;
};
