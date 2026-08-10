#pragma once

struct NetCommandUpdateEvent
{
	double deltaTime;
};

struct PreTickUpdateEvent
{
	double deltaTime;
};

struct TickUpdateEvent
{
	double deltaTime;
};

struct PostTickUpdateEvent
{
	double deltaTime;
};

struct DeltaTimeEvent
{
	double deltaTime;
};
