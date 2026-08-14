#pragma once

// Unreal-style native text character and literal support.
// On Windows, wchar_t is the native wide-character type used by TEXT("...").
#ifndef _TCHAR_DEFINED
using TCHAR = wchar_t;
#define _TCHAR_DEFINED
#endif

#ifndef TEXT
#define UE_TEXT_LITERAL_IMPL(Value) L##Value
#define TEXT(Value) UE_TEXT_LITERAL_IMPL(Value)
#endif

#define DECLARE_SINGLETON(_UserDefineDataType)      friend class TSingleton<_UserDefineDataType>;   \
                                                    private:                                        \
                                                        _UserDefineDataType() = default;            \
                                                        ~_UserDefineDataType() = default;

#ifdef _MSC_VER
    #define FORCEINLINE __forceinline
#elif defined(__GNUC__) || defined(__clang__)
    #define FORCEINLINE __attribute__((always_inline)) inline
#else
    #define FORCEINLINE inline
#endif

#define Super       __super

#define ABSTRACT    abstract

// Unreal Header Tool style annotations. They are compile-time no-ops until
// reflection/code generation is implemented, but keep gameplay declarations
// source-compatible with Unreal's annotation layout.
#define UCLASS(...)
#define USTRUCT(...)
#define UFUNCTION(...)
#define UPROPERTY(...)
#define UENUM(...)
#define UMETA(...)
#define GENERATED_BODY(...)
#define GENERATED_UCLASS_BODY(...)

#define ENGINE_API
#define PROJECT_API

#ifndef check
#define check(Expression) assert(Expression)
#endif

#ifndef checkSlow
#define checkSlow(Expression) assert(Expression)
#endif

#ifndef checkf
#define checkf(Expression, Format, ...) assert(Expression)
#endif

#ifndef ensure
#define ensure(Expression) (!!(Expression))
#endif

#ifndef ensureMsgf
#define ensureMsgf(Expression, Format, ...) (!!(Expression))
#endif
