#pragma once

template <typename ReturnType, typename... ParameterTypes>
template <typename UserClass>
void TDelegate<ReturnType(ParameterTypes...)>::BindRaw(
	UserClass* _Object, ReturnType(UserClass::* _Function)(ParameterTypes...))
{
	BindLambda([_Object, _Function](ParameterTypes... _Parameters) -> ReturnType
	{
		return (_Object->*_Function)(std::forward<ParameterTypes>(_Parameters)...);
	});
}

template <typename ReturnType, typename... ParameterTypes>
template <typename UserClass>
void TDelegate<ReturnType(ParameterTypes...)>::BindRaw(
	const UserClass* _Object, ReturnType(UserClass::* _Function)(ParameterTypes...) const)
{
	BindLambda([_Object, _Function](ParameterTypes... _Parameters) -> ReturnType
	{
		return (_Object->*_Function)(std::forward<ParameterTypes>(_Parameters)...);
	});
}

template <typename ReturnType, typename... ParameterTypes>
void TDelegate<ReturnType(ParameterTypes...)>::BindStatic(
	ReturnType(*_Function)(ParameterTypes...))
{
	BindLambda(_Function);
}

template <typename ReturnType, typename... ParameterTypes>
template <typename CallableType>
void TDelegate<ReturnType(ParameterTypes...)>::BindLambda(CallableType&& _Callable)
{
	__Function = std::forward<CallableType>(_Callable);
	__Handle = FDelegateHandle::GenerateNewHandle();
}

template <typename ReturnType, typename... ParameterTypes>
void TDelegate<ReturnType(ParameterTypes...)>::Unbind()
{
	__Function = nullptr;
	__Handle.Reset();
}

template <typename ReturnType, typename... ParameterTypes>
bool TDelegate<ReturnType(ParameterTypes...)>::IsBound() const
{
	return static_cast<bool>(__Function);
}

template <typename ReturnType, typename... ParameterTypes>
FDelegateHandle TDelegate<ReturnType(ParameterTypes...)>::GetHandle() const
{
	return __Handle;
}

template <typename ReturnType, typename... ParameterTypes>
ReturnType TDelegate<ReturnType(ParameterTypes...)>::Execute(ParameterTypes... _Parameters) const
{
	assert(IsBound());
	return __Function(std::forward<ParameterTypes>(_Parameters)...);
}

template <typename ReturnType, typename... ParameterTypes>
template <typename Type>
std::enable_if_t<std::is_void_v<Type>, bool>
TDelegate<ReturnType(ParameterTypes...)>::ExecuteIfBound(ParameterTypes... _Parameters) const
{
	if (false == IsBound())
		return false;

	__Function(std::forward<ParameterTypes>(_Parameters)...);
	return true;
}
