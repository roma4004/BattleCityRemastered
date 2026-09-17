#pragma once

//NOTE: which still sprite an entity draws, not its place in the atlas
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
