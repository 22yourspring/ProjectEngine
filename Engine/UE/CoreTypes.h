#pragma once

#include "define.h"

#include <cstddef>
#include <cstdint>
#include <type_traits>

using int8 = std::int8_t;
using int16 = std::int16_t;
using int32 = std::int32_t;
using int64 = std::int64_t;

using uint8 = std::uint8_t;
using uint16 = std::uint16_t;
using uint32 = std::uint32_t;
using uint64 = std::uint64_t;

/**
 * Explicit zero/identity initialization tag used by Unreal math types.
 */
enum EForceInit
{
	ForceInit,
	ForceInitToZero
};

enum ENoInit
{
	NoInit
};

inline constexpr int32 INDEX_NONE = -1;

#ifndef CORE_API
#define CORE_API
#endif

#ifndef UE_FORCEINLINE_HINT
#define UE_FORCEINLINE_HINT FORCEINLINE
#endif

#ifndef FORCEINLINE_DEBUGGABLE
#define FORCEINLINE_DEBUGGABLE inline
#endif

#ifndef RESTRICT
#if defined(_MSC_VER)
#define RESTRICT __restrict
#elif defined(__GNUC__) || defined(__clang__)
#define RESTRICT __restrict__
#else
#define RESTRICT
#endif
#endif

#ifndef UE_REQUIRES
#define UE_REQUIRES(...) , std::enable_if_t<(__VA_ARGS__), int> = 0
#endif

#ifndef UE_DEPRECATED
#define UE_DEPRECATED(Version, Message)
#endif

#ifndef PRAGMA_DISABLE_DEPRECATION_WARNINGS
#define PRAGMA_DISABLE_DEPRECATION_WARNINGS
#define PRAGMA_ENABLE_DEPRECATION_WARNINGS
#endif

#ifndef PRAGMA_DISABLE_SWITCH_UNHANDLED_ENUM_CASE_WARNINGS
#define PRAGMA_DISABLE_SWITCH_UNHANDLED_ENUM_CASE_WARNINGS
#define PRAGMA_RESTORE_SWITCH_UNHANDLED_ENUM_CASE_WARNINGS
#endif

#ifndef UE_DECLARE_LWC_TYPE
#define UE_DECLARE_LWC_TYPE(...)
#endif

template<typename T> struct TIsPODType;
template<typename T> struct TIsUECoreVariant;

#ifndef UE_BUILD_SHIPPING
#define UE_BUILD_SHIPPING 0
#endif

#ifndef UE_BUILD_TEST
#define UE_BUILD_TEST 0
#endif

#ifndef WITH_EDITORONLY_DATA
#define WITH_EDITORONLY_DATA 0
#endif

#ifndef ENABLE_NAN_DIAGNOSTIC
#define ENABLE_NAN_DIAGNOSTIC 0
#endif

#ifndef PLATFORM_ENABLE_VECTORINTRINSICS
#define PLATFORM_ENABLE_VECTORINTRINSICS 0
#endif

#ifndef PLATFORM_ENABLE_VECTORINTRINSICS_NEON
#define PLATFORM_ENABLE_VECTORINTRINSICS_NEON 0
#endif

#ifndef PLATFORM_ALWAYS_HAS_SSE4_1
#define PLATFORM_ALWAYS_HAS_SSE4_1 0
#endif

#ifndef PLATFORM_ALWAYS_HAS_AVX
#define PLATFORM_ALWAYS_HAS_AVX 0
#endif

#ifndef PLATFORM_ALWAYS_HAS_AVX_2
#define PLATFORM_ALWAYS_HAS_AVX_2 0
#endif

#ifndef PLATFORM_ALWAYS_HAS_FMA3
#define PLATFORM_ALWAYS_HAS_FMA3 0
#endif

#ifndef PLATFORM_LITTLE_ENDIAN
#define PLATFORM_LITTLE_ENDIAN 1
#endif

#ifndef PLATFORM_SUPPORTS_VECTOR_CONTROL_REGISTERS
#define PLATFORM_SUPPORTS_VECTOR_CONTROL_REGISTERS 0
#endif

#ifndef ENABLE_VECTORIZED_TRANSFORM
#define ENABLE_VECTORIZED_TRANSFORM PLATFORM_ENABLE_VECTORINTRINSICS
#endif
