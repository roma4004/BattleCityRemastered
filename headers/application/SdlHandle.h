#pragma once

#include <memory>

//NOTE: the destructor is part of the type - no function pointer stored per handle, and none can be null
template<class T, void (*DeleteFn)(T*)>
struct SdlDeleter
{
	void operator()(T* resource) const noexcept { DeleteFn(resource); }
};

//NOTE: T is spelled out beside DeleteFn - ReSharper C++ cannot resolve it deduced and marks every alias red
template<class T, void (*DeleteFn)(T*)>
using SdlHandle = std::unique_ptr<T, SdlDeleter<T, DeleteFn>>;
