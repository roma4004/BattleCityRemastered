#pragma once

#include "BrickWall.h"
#include "IFortress.h"
#include "SteelWall.h"

class FortressBrickWall final : public BrickWall, public IFortress
{
public:
	using BrickWall::BrickWall;
};

class FortressSteelWall final : public SteelWall, public IFortress
{
public:
	using SteelWall::SteelWall;
};
