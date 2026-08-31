#include "utils/ObjectUtils.h"
#include "entities/BaseObj.h"

bool ObjectUtils::IsAlive(const std::shared_ptr<BaseObj>& object)
{
	return object != nullptr && object->GetIsAlive();
}
