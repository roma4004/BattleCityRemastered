#pragma once

class IObsticle {
public:
	virtual ~IObsticle() = default;

	virtual bool IsPassable() = 0;
	virtual bool IsDestructible() = 0;
	virtual bool IsPenetrating() = 0;
};
