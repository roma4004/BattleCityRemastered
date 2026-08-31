#pragma once

#include <memory>

//NOTE: the destructor belongs to the type, not to the value - decltype(&SDL_DestroyX) as a deleter
//stores a function pointer per handle and lets one be built with a null deleter
template<class T, void (*DeleteFn)(T*)>
struct SdlDeleter
{
	void operator()(T* resource) const noexcept { DeleteFn(resource); }
};

//NOTE: the type is spelled out beside its destructor rather than deduced from it - the deduced form
//compiles everywhere, but ReSharper C++ cannot resolve it and marks every alias red
template<class T, void (*DeleteFn)(T*)>
using SdlHandle = std::unique_ptr<T, SdlDeleter<T, DeleteFn>>;
