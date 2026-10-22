#pragma once

#include "../entities/BaseObj.h"
#include "utils/Uuid.h"

enum class Direction : char8_t;
enum class AnimationType : char8_t;

//NOTE: runs until something disposes of it - the water, a client's spawn burst
inline constexpr int kEndlessAnimation{0};

class AnimatedObject final
{
public:
	ObjRectangle rect{};
	int currentFrameIndex{};
	int ticksSinceLastFrame{};
	int size{};
	int speed{};//NOTE: ticks between frame advances
	int passes{1};//NOTE: how many times the frame sequence is played before disposal
	int passesDone{};
	Uuid owner{};//NOTE: the spawn this animation runs for, if any - it is told when the last frame is done
	Direction dir{};
	AnimationType type{};
	bool markToDispose{};
	int scale{};
};
