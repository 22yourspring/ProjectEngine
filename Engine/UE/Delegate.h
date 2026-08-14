#pragma once

#include <atomic>
#include <cassert>
#include <cstdint>
#include <functional>
#include <type_traits>
#include <utility>


class FDelegateHandle
{
public:
	FDelegateHandle() = default;

	bool IsValid() const { return 0 != __ID; }
	void Reset() { __ID = 0; }

	bool operator==(const FDelegateHandle& _Other) const { return __ID == _Other.__ID; }
	bool operator!=(const FDelegateHandle& _Other) const { return false == (*this == _Other); }

	static FDelegateHandle GenerateNewHandle()
	{
		return FDelegateHandle(__NextID.fetch_add(1, std::memory_order_relaxed));
	}

private:
	explicit FDelegateHandle(uint64_t _ID) : __ID(_ID) {}

	uint64_t __ID = 0;
	inline static std::atomic_uint64_t __NextID = 1;
};


template <typename Signature>
class TDelegate;

template <typename ReturnType, typename... ParameterTypes>
class TDelegate<ReturnType(ParameterTypes...)>
{
public:
	TDelegate() = default;

	template <typename UserClass>
	void BindRaw(UserClass* _Object, ReturnType(UserClass::* _Function)(ParameterTypes...));

	template <typename UserClass>
	void BindRaw(const UserClass* _Object, ReturnType(UserClass::* _Function)(ParameterTypes...) const);

	void BindStatic(ReturnType(*_Function)(ParameterTypes...));

	template <typename CallableType>
	void BindLambda(CallableType&& _Callable);

	void Unbind();

	bool IsBound() const;
	FDelegateHandle GetHandle() const;

	ReturnType Execute(ParameterTypes... _Parameters) const;

	template <typename Type = ReturnType>
	std::enable_if_t<std::is_void_v<Type>, bool> ExecuteIfBound(ParameterTypes... _Parameters) const;

private:
	std::function<ReturnType(ParameterTypes...)> __Function;
	FDelegateHandle __Handle;
};

#include "Delegate.inl"
