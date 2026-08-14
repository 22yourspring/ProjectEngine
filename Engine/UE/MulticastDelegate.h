#pragma once

#include "Delegate.h"

#include <algorithm>
#include <functional>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>


template <typename Signature>
class TMulticastDelegate;

template <typename... ParameterTypes>
class TMulticastDelegate<void(ParameterTypes...)>
{
private:
	struct FBinding
	{
		FDelegateHandle __Handle;
		std::function<void(ParameterTypes...)> __Function;
		const void* __Object = nullptr;
		bool __bActive = true;
	};

public:
	template <typename UserClass>
	FDelegateHandle AddRaw(UserClass* _Object, void(UserClass::* _Function)(ParameterTypes...));

	template <typename UserClass>
	FDelegateHandle AddRaw(const UserClass* _Object, void(UserClass::* _Function)(ParameterTypes...) const);

	FDelegateHandle AddStatic(void(*_Function)(ParameterTypes...));

	template <typename CallableType>
	FDelegateHandle AddLambda(CallableType&& _Callable);

	bool Remove(FDelegateHandle _Handle);

	void RemoveAll(const void* _Object);

	void Clear();

	bool IsBound() const;

	void Broadcast(ParameterTypes... _Parameters);

private:
	template <typename CallableType>
	FDelegateHandle AddInternal(CallableType&& _Callable, const void* _Object);

	void Compact();

	std::vector<std::shared_ptr<FBinding>> __Bindings;
	uint32_t __BroadcastDepth = 0;
};

#include "MulticastDelegate.inl"
