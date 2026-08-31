#include "components/AnimatedObjects.h"
#include "enums/AnimationType.h"

AnimatedObject::AnimatedObject(const std::string& objName, const ObjRectangle objRect, const AnimationType objType,
							   const int frameLimit, const int objScale, const int animationSpeed, const int passCount,
							   const Uuid objOwner)
	: rect{objRect}
	, size{frameLimit}
	, speed{animationSpeed}
	, passes{passCount}
	, owner{objOwner}
	, type(objType)
	, scale{objScale}
	, name{objName} {}
