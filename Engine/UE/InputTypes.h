#pragma once

#include <cstdint>
#include <string>
#include <string_view>


UENUM()
enum class EKey : uint16_t
{
	Invalid = 0,

	Escape,
	F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12,
	F13, F14, F15, F16, F17, F18, F19, F20, F21, F22, F23, F24,
	PrintScreen,
	ScrollLock,
	Pause,

	Grave,
	Num1, Num2, Num3, Num4, Num5, Num6, Num7, Num8, Num9, Num0,
	Minus,
	Equals,
	Backspace,
	Tab,
	Q, W, E, R, T, Y, U, I, O, P,
	LeftBracket,
	RightBracket,
	Backslash,
	CapsLock,
	A, S, D, F, G, H, J, K, L,
	Semicolon,
	Apostrophe,
	Enter,
	LeftShift,
	Z, X, C, V, B, N, M,
	Comma,
	Period,
	Slash,
	RightShift,
	LeftControl,
	LeftWindows,
	LeftAlt,
	SpaceBar,
	RightAlt,
	RightWindows,
	Application,
	RightControl,

	Insert,
	Delete,
	Home,
	End,
	PageUp,
	PageDown,
	Left,
	Right,
	Up,
	Down,

	NumLock,
	NumpadDivide,
	NumpadMultiply,
	NumpadSubtract,
	NumpadAdd,
	NumpadDecimal,
	Numpad0, Numpad1, Numpad2, Numpad3, Numpad4,
	Numpad5, Numpad6, Numpad7, Numpad8, Numpad9,

	Hangul,
	Hanja,
	OEM102,

	BrowserBack,
	BrowserForward,
	BrowserRefresh,
	BrowserStop,
	BrowserSearch,
	BrowserFavorites,
	BrowserHome,
	VolumeMute,
	VolumeDown,
	VolumeUp,
	MediaNextTrack,
	MediaPreviousTrack,
	MediaStop,
	MediaPlayPause,
	LaunchMail,
	LaunchMediaSelect,
	LaunchApplication1,
	LaunchApplication2,

	LeftMouseButton,
	RightMouseButton,
	MiddleMouseButton,
	ThumbMouseButton,
	ThumbMouseButton2,
	MouseX,
	MouseY,
	MouseWheelAxis
};


UENUM()
enum class EInputEvent : uint8_t
{
	Pressed,
	Released,
	Repeat
};


USTRUCT()
struct FInputActionKeyMapping
{
	GENERATED_BODY()

	UPROPERTY()
	std::string __ActionName;
	UPROPERTY()
	EKey __Key = EKey::Invalid;
};


USTRUCT()
struct FInputAxisKeyMapping
{
	GENERATED_BODY()

	UPROPERTY()
	std::string __AxisName;
	UPROPERTY()
	EKey __Key = EKey::Invalid;
	UPROPERTY()
	float __Scale = 0.0f;
};


USTRUCT()
struct FKeyInputEvent
{
	GENERATED_BODY()

	EKey __Key = EKey::Invalid;
	EInputEvent __Event = EInputEvent::Pressed;
};

USTRUCT()
struct FAnalogInputEvent
{
	GENERATED_BODY()

	EKey __Key = EKey::Invalid;
	float __Value = 0.0f;
};


struct FKeyNameEntry
{
	EKey __Key;
	std::string_view __Name;
};

#define KEY_NAME_ENTRY(KeyName) { EKey::KeyName, #KeyName }

inline constexpr FKeyNameEntry KeyNameEntries[] =
{
	KEY_NAME_ENTRY(Invalid),
	KEY_NAME_ENTRY(Escape),
	KEY_NAME_ENTRY(F1), KEY_NAME_ENTRY(F2), KEY_NAME_ENTRY(F3), KEY_NAME_ENTRY(F4),
	KEY_NAME_ENTRY(F5), KEY_NAME_ENTRY(F6), KEY_NAME_ENTRY(F7), KEY_NAME_ENTRY(F8),
	KEY_NAME_ENTRY(F9), KEY_NAME_ENTRY(F10), KEY_NAME_ENTRY(F11), KEY_NAME_ENTRY(F12),
	KEY_NAME_ENTRY(F13), KEY_NAME_ENTRY(F14), KEY_NAME_ENTRY(F15), KEY_NAME_ENTRY(F16),
	KEY_NAME_ENTRY(F17), KEY_NAME_ENTRY(F18), KEY_NAME_ENTRY(F19), KEY_NAME_ENTRY(F20),
	KEY_NAME_ENTRY(F21), KEY_NAME_ENTRY(F22), KEY_NAME_ENTRY(F23), KEY_NAME_ENTRY(F24),
	KEY_NAME_ENTRY(PrintScreen), KEY_NAME_ENTRY(ScrollLock), KEY_NAME_ENTRY(Pause),
	KEY_NAME_ENTRY(Grave),
	KEY_NAME_ENTRY(Num1), KEY_NAME_ENTRY(Num2), KEY_NAME_ENTRY(Num3), KEY_NAME_ENTRY(Num4),
	KEY_NAME_ENTRY(Num5), KEY_NAME_ENTRY(Num6), KEY_NAME_ENTRY(Num7), KEY_NAME_ENTRY(Num8),
	KEY_NAME_ENTRY(Num9), KEY_NAME_ENTRY(Num0),
	KEY_NAME_ENTRY(Minus), KEY_NAME_ENTRY(Equals), KEY_NAME_ENTRY(Backspace), KEY_NAME_ENTRY(Tab),
	KEY_NAME_ENTRY(Q), KEY_NAME_ENTRY(W), KEY_NAME_ENTRY(E), KEY_NAME_ENTRY(R), KEY_NAME_ENTRY(T),
	KEY_NAME_ENTRY(Y), KEY_NAME_ENTRY(U), KEY_NAME_ENTRY(I), KEY_NAME_ENTRY(O), KEY_NAME_ENTRY(P),
	KEY_NAME_ENTRY(LeftBracket), KEY_NAME_ENTRY(RightBracket), KEY_NAME_ENTRY(Backslash),
	KEY_NAME_ENTRY(CapsLock),
	KEY_NAME_ENTRY(A), KEY_NAME_ENTRY(S), KEY_NAME_ENTRY(D), KEY_NAME_ENTRY(F), KEY_NAME_ENTRY(G),
	KEY_NAME_ENTRY(H), KEY_NAME_ENTRY(J), KEY_NAME_ENTRY(K), KEY_NAME_ENTRY(L),
	KEY_NAME_ENTRY(Semicolon), KEY_NAME_ENTRY(Apostrophe), KEY_NAME_ENTRY(Enter),
	KEY_NAME_ENTRY(LeftShift),
	KEY_NAME_ENTRY(Z), KEY_NAME_ENTRY(X), KEY_NAME_ENTRY(C), KEY_NAME_ENTRY(V), KEY_NAME_ENTRY(B),
	KEY_NAME_ENTRY(N), KEY_NAME_ENTRY(M),
	KEY_NAME_ENTRY(Comma), KEY_NAME_ENTRY(Period), KEY_NAME_ENTRY(Slash), KEY_NAME_ENTRY(RightShift),
	KEY_NAME_ENTRY(LeftControl), KEY_NAME_ENTRY(LeftWindows), KEY_NAME_ENTRY(LeftAlt),
	KEY_NAME_ENTRY(SpaceBar), KEY_NAME_ENTRY(RightAlt), KEY_NAME_ENTRY(RightWindows),
	KEY_NAME_ENTRY(Application), KEY_NAME_ENTRY(RightControl),
	KEY_NAME_ENTRY(Insert), KEY_NAME_ENTRY(Delete), KEY_NAME_ENTRY(Home), KEY_NAME_ENTRY(End),
	KEY_NAME_ENTRY(PageUp), KEY_NAME_ENTRY(PageDown), KEY_NAME_ENTRY(Left), KEY_NAME_ENTRY(Right),
	KEY_NAME_ENTRY(Up), KEY_NAME_ENTRY(Down),
	KEY_NAME_ENTRY(NumLock), KEY_NAME_ENTRY(NumpadDivide), KEY_NAME_ENTRY(NumpadMultiply),
	KEY_NAME_ENTRY(NumpadSubtract), KEY_NAME_ENTRY(NumpadAdd), KEY_NAME_ENTRY(NumpadDecimal),
	KEY_NAME_ENTRY(Numpad0), KEY_NAME_ENTRY(Numpad1), KEY_NAME_ENTRY(Numpad2),
	KEY_NAME_ENTRY(Numpad3), KEY_NAME_ENTRY(Numpad4), KEY_NAME_ENTRY(Numpad5),
	KEY_NAME_ENTRY(Numpad6), KEY_NAME_ENTRY(Numpad7), KEY_NAME_ENTRY(Numpad8), KEY_NAME_ENTRY(Numpad9),
	KEY_NAME_ENTRY(Hangul), KEY_NAME_ENTRY(Hanja), KEY_NAME_ENTRY(OEM102),
	KEY_NAME_ENTRY(BrowserBack), KEY_NAME_ENTRY(BrowserForward), KEY_NAME_ENTRY(BrowserRefresh),
	KEY_NAME_ENTRY(BrowserStop), KEY_NAME_ENTRY(BrowserSearch), KEY_NAME_ENTRY(BrowserFavorites),
	KEY_NAME_ENTRY(BrowserHome), KEY_NAME_ENTRY(VolumeMute), KEY_NAME_ENTRY(VolumeDown),
	KEY_NAME_ENTRY(VolumeUp), KEY_NAME_ENTRY(MediaNextTrack), KEY_NAME_ENTRY(MediaPreviousTrack),
	KEY_NAME_ENTRY(MediaStop), KEY_NAME_ENTRY(MediaPlayPause), KEY_NAME_ENTRY(LaunchMail),
	KEY_NAME_ENTRY(LaunchMediaSelect), KEY_NAME_ENTRY(LaunchApplication1),
	KEY_NAME_ENTRY(LaunchApplication2),
	KEY_NAME_ENTRY(LeftMouseButton), KEY_NAME_ENTRY(RightMouseButton),
	KEY_NAME_ENTRY(MiddleMouseButton), KEY_NAME_ENTRY(ThumbMouseButton),
	KEY_NAME_ENTRY(ThumbMouseButton2), KEY_NAME_ENTRY(MouseX), KEY_NAME_ENTRY(MouseY),
	KEY_NAME_ENTRY(MouseWheelAxis)
};

#undef KEY_NAME_ENTRY

static_assert(
	std::size(KeyNameEntries) == static_cast<size_t>(EKey::MouseWheelAxis) + 1,
	"Every EKey must have a serialized name.");

inline std::string_view GetKeyName(EKey _Key)
{
	const size_t Index = static_cast<size_t>(_Key);
	if (Index < std::size(KeyNameEntries) && KeyNameEntries[Index].__Key == _Key)
		return KeyNameEntries[Index].__Name;

	return "Invalid";
}

inline bool TryParseKey(std::string_view _Name, EKey& _OutKey)
{
	for (const FKeyNameEntry& Entry : KeyNameEntries)
	{
		if (Entry.__Name == _Name)
		{
			_OutKey = Entry.__Key;
			return true;
		}
	}

	constexpr FKeyNameEntry LegacyNumberKeyNames[] =
	{
		{ EKey::Num0, "Zero" }, { EKey::Num1, "One" }, { EKey::Num2, "Two" },
		{ EKey::Num3, "Three" }, { EKey::Num4, "Four" }, { EKey::Num5, "Five" },
		{ EKey::Num6, "Six" }, { EKey::Num7, "Seven" }, { EKey::Num8, "Eight" },
		{ EKey::Num9, "Nine" }
	};

	for (const FKeyNameEntry& Entry : LegacyNumberKeyNames)
	{
		if (Entry.__Name == _Name)
		{
			_OutKey = Entry.__Key;
			return true;
		}
	}

	_OutKey = EKey::Invalid;
	return false;
}
