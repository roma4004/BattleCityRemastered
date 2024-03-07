#pragma once

class IObsticle
{
public:
	virtual ~IObsticle() = default;

	virtual bool GetIsPassable() = 0;
	virtual bool GetIsDestructible() = 0;
	virtual bool GetIsPenetrable() = 0;
	virtual void SetIsPassable(bool value) = 0;
	virtual void SetIsDestructible(bool value) = 0;
	virtual void SetIsPenetrable(bool value) = 0;

protected:
	bool _isPassable = false;
	bool _isDestructible = false;
	bool _isPenetrable = false;
};