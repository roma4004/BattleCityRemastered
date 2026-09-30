#pragma once

#include "../entities/BaseObj.h"
#include "enums/Author.h"
#include "enums/TankModel.h"
#include "utils/Uuid.h"

enum class Direction : char8_t;
enum class AnimationType : char8_t;

//NOTE: an animation that runs until something disposes of it - tracks, water, the helmet halo
inline constexpr int kEndlessAnimation{0};

class AnimatedObject final
{
public:
	ObjRectangle rect{};
	Direction dir{};
	int currentFrameIndex{};//NOTE: frame index currently shown, advances every animationSpeed ticks
	int ticksSinceLastFrame{};//NOTE: tick counter throttling how often currentFrameIndex advances
	int size{};
	int speed{};//NOTE: ticks between frame advances;
	int passes{1};//NOTE: how many times the frame sequence is played before disposal
	int passesDone{};
	Uuid owner{};//NOTE: the spawn this animation runs for, if any - it is told when the last frame is done
	AnimationType type{};
	bool markToDispose{};
	int scale{};
	Author author{};
	//NOTE: read by the tank animations alone - the others are drawn from the type and ignore it
	TankModel model{};
	unsigned short tier{1u};
};
