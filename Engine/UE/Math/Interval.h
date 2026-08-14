#pragma once

#include "UE/CoreTypes.h"

namespace UE::Math
{
	template <typename T>
	struct TInterval
	{
		T Min = {};
		T Max = {};

		constexpr TInterval() = default;

		constexpr TInterval(T _Min, T _Max)
			: Min(_Min)
			, Max(_Max)
		{
		}

		[[nodiscard]] constexpr T Size() const
		{
			return Max - Min;
		}

		[[nodiscard]] constexpr bool Contains(T _Value) const
		{
			return _Value >= Min && _Value <= Max;
		}

		[[nodiscard]] constexpr bool IsValid() const
		{
			return Min <= Max;
		}
	};
}

using FFloatInterval = UE::Math::TInterval<float>;
