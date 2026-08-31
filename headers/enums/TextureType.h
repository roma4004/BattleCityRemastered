#pragma once

//NOTE: which sprite an entity draws, not where it sits in the atlas - that mapping is TextureManager's.
//The still half of what AnimationType already does for the animated one
enum class TextureType : char8_t
{
	None,

	Bullet,
	Eagle,
	BrickWall,
	SteelWall,
	Bush,
	Ice,

	BonusTimer,
	BonusHelmet,
	BonusGrenade,
	BonusTank,
	BonusStar,
	BonusShovel,
	BonusCaliber,
	BonusShip,
};
