#pragma once

template <typename... ParameterTypes>
template <typename UserClass>
FDelegateHandle TMulticastDelegate<void(ParameterTypes...)>::AddRaw(
	UserClass* _Object, void(UserClass::* _Function)(ParameterTypes...))
{
	return AddInternal([_Object, _Function](ParameterTypes... _Parameters)
	{
		(_Object->*_Function)(std::forward<ParameterTypes>(_Parameters)...);
	}, _Object);
}

template <typename... ParameterTypes>
template <typename UserClass>
FDelegateHandle TMulticastDelegate<void(ParameterTypes...)>::AddRaw(
	const UserClass* _Object, void(UserClass::* _Function)(ParameterTypes...) const)
{
	return AddInternal([_Object, _Function](ParameterTypes... _Parameters)
	{
		(_Object->*_Function)(std::forward<ParameterTypes>(_Parameters)...);
	}, _Object);
}

template <typename... ParameterTypes>
FDelegateHandle TMulticastDelegate<void(ParameterTypes...)>::AddStatic(
	void(*_Function)(ParameterTypes...))
{
	return AddInternal(_Function, nullptr);
}

template <typename... ParameterTypes>
template <typename CallableType>
FDelegateHandle TMulticastDelegate<void(ParameterTypes...)>::AddLambda(CallableType&& _Callable)
{
	return AddInternal(std::forward<CallableType>(_Callable), nullptr);
}

template <typename... ParameterTypes>
bool TMulticastDelegate<void(ParameterTypes...)>::Remove(FDelegateHandle _Handle)
{
	for (const std::shared_ptr<FBinding>& Binding : __Bindings)
	{
		if (Binding->__Handle == _Handle)
		{
			Binding->__bActive = false;
			Compact();
			return true;
		}
	}
	return false;
}

template <typename... ParameterTypes>
void TMulticastDelegate<void(ParameterTypes...)>::RemoveAll(const void* _Object)
{
	for (const std::shared_ptr<FBinding>& Binding : __Bindings)
	{
		if (Binding->__Object == _Object)
			Binding->__bActive = false;
	}
	Compact();
}

template <typename... ParameterTypes>
void TMulticastDelegate<void(ParameterTypes...)>::Clear()
{
	for (const std::shared_ptr<FBinding>& Binding : __Bindings)
		Binding->__bActive = false;
	Compact();
}

template <typename... ParameterTypes>
bool TMulticastDelegate<void(ParameterTypes...)>::IsBound() const
{
	return std::any_of(__Bindings.begin(), __Bindings.end(),
		[](const std::shared_ptr<FBinding>& _Binding) { return _Binding->__bActive; });
}

template <typename... ParameterTypes>
void TMulticastDelegate<void(ParameterTypes...)>::Broadcast(ParameterTypes... _Parameters)
{
	const std::vector<std::shared_ptr<FBinding>> Snapshot = __Bindings;
	++__BroadcastDepth;
	for (const std::shared_ptr<FBinding>& Binding : Snapshot)
	{
		if (Binding->__bActive)
			Binding->__Function(_Parameters...);
	}
	--__BroadcastDepth;
	Compact();
}

template <typename... ParameterTypes>
template <typename CallableType>
FDelegateHandle TMulticastDelegate<void(ParameterTypes...)>::AddInternal(
	CallableType&& _Callable, const void* _Object)
{
	const FDelegateHandle Handle = FDelegateHandle::GenerateNewHandle();
	auto Binding = std::make_shared<FBinding>();
	Binding->__Handle = Handle;
	Binding->__Function = std::forward<CallableType>(_Callable);
	Binding->__Object = _Object;
	__Bindings.emplace_back(std::move(Binding));
	return Handle;
}

template <typename... ParameterTypes>
void TMulticastDelegate<void(ParameterTypes...)>::Compact()
{
	if (0 != __BroadcastDepth)
		return;

	__Bindings.erase(std::remove_if(__Bindings.begin(), __Bindings.end(),
		[](const std::shared_ptr<FBinding>& _Binding) { return false == _Binding->__bActive; }),
		__Bindings.end());
}
