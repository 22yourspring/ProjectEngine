#include "pch.h"
#include "InputSubsystem.h"

#include "PlayerController.h"
#include "WindowsInputApplication.h"

namespace
{
	struct FPolledKey
	{
		EKey __Key;
		int __VirtualKey;
	};

	constexpr FPolledKey PolledKeys[] =
	{
		{ EKey::Escape, VK_ESCAPE },
		{ EKey::F1, VK_F1 }, { EKey::F2, VK_F2 }, { EKey::F3, VK_F3 },
		{ EKey::F4, VK_F4 }, { EKey::F5, VK_F5 }, { EKey::F6, VK_F6 },
		{ EKey::F7, VK_F7 }, { EKey::F8, VK_F8 }, { EKey::F9, VK_F9 },
		{ EKey::F10, VK_F10 }, { EKey::F11, VK_F11 }, { EKey::F12, VK_F12 },
		{ EKey::F13, VK_F13 }, { EKey::F14, VK_F14 }, { EKey::F15, VK_F15 },
		{ EKey::F16, VK_F16 }, { EKey::F17, VK_F17 }, { EKey::F18, VK_F18 },
		{ EKey::F19, VK_F19 }, { EKey::F20, VK_F20 }, { EKey::F21, VK_F21 },
		{ EKey::F22, VK_F22 }, { EKey::F23, VK_F23 }, { EKey::F24, VK_F24 },
		{ EKey::PrintScreen, VK_SNAPSHOT }, { EKey::ScrollLock, VK_SCROLL },
		{ EKey::Pause, VK_PAUSE },

		{ EKey::Grave, VK_OEM_3 },
		{ EKey::Num0, '0' }, { EKey::Num1, '1' }, { EKey::Num2, '2' },
		{ EKey::Num3, '3' }, { EKey::Num4, '4' }, { EKey::Num5, '5' },
		{ EKey::Num6, '6' }, { EKey::Num7, '7' }, { EKey::Num8, '8' },
		{ EKey::Num9, '9' }, { EKey::Minus, VK_OEM_MINUS },
		{ EKey::Equals, VK_OEM_PLUS }, { EKey::Backspace, VK_BACK },
		{ EKey::Tab, VK_TAB },
		{ EKey::Q, 'Q' }, { EKey::W, 'W' }, { EKey::E, 'E' },
		{ EKey::R, 'R' }, { EKey::T, 'T' }, { EKey::Y, 'Y' },
		{ EKey::U, 'U' }, { EKey::I, 'I' }, { EKey::O, 'O' }, { EKey::P, 'P' },
		{ EKey::LeftBracket, VK_OEM_4 }, { EKey::RightBracket, VK_OEM_6 },
		{ EKey::Backslash, VK_OEM_5 }, { EKey::CapsLock, VK_CAPITAL },
		{ EKey::A, 'A' }, { EKey::S, 'S' }, { EKey::D, 'D' },
		{ EKey::F, 'F' }, { EKey::G, 'G' }, { EKey::H, 'H' },
		{ EKey::J, 'J' }, { EKey::K, 'K' }, { EKey::L, 'L' },
		{ EKey::Semicolon, VK_OEM_1 }, { EKey::Apostrophe, VK_OEM_7 },
		{ EKey::Enter, VK_RETURN }, { EKey::LeftShift, VK_LSHIFT },
		{ EKey::Z, 'Z' }, { EKey::X, 'X' }, { EKey::C, 'C' },
		{ EKey::V, 'V' }, { EKey::B, 'B' }, { EKey::N, 'N' }, { EKey::M, 'M' },
		{ EKey::Comma, VK_OEM_COMMA }, { EKey::Period, VK_OEM_PERIOD },
		{ EKey::Slash, VK_OEM_2 }, { EKey::RightShift, VK_RSHIFT },
		{ EKey::LeftControl, VK_LCONTROL }, { EKey::LeftWindows, VK_LWIN },
		{ EKey::LeftAlt, VK_LMENU }, { EKey::SpaceBar, VK_SPACE },
		{ EKey::RightAlt, VK_RMENU }, { EKey::RightWindows, VK_RWIN },
		{ EKey::Application, VK_APPS }, { EKey::RightControl, VK_RCONTROL },

		{ EKey::Insert, VK_INSERT }, { EKey::Delete, VK_DELETE },
		{ EKey::Home, VK_HOME }, { EKey::End, VK_END },
		{ EKey::PageUp, VK_PRIOR }, { EKey::PageDown, VK_NEXT },
		{ EKey::Left, VK_LEFT }, { EKey::Right, VK_RIGHT },
		{ EKey::Up, VK_UP }, { EKey::Down, VK_DOWN },

		{ EKey::NumLock, VK_NUMLOCK }, { EKey::NumpadDivide, VK_DIVIDE },
		{ EKey::NumpadMultiply, VK_MULTIPLY }, { EKey::NumpadSubtract, VK_SUBTRACT },
		{ EKey::NumpadAdd, VK_ADD }, { EKey::NumpadDecimal, VK_DECIMAL },
		{ EKey::Numpad0, VK_NUMPAD0 }, { EKey::Numpad1, VK_NUMPAD1 },
		{ EKey::Numpad2, VK_NUMPAD2 }, { EKey::Numpad3, VK_NUMPAD3 },
		{ EKey::Numpad4, VK_NUMPAD4 }, { EKey::Numpad5, VK_NUMPAD5 },
		{ EKey::Numpad6, VK_NUMPAD6 }, { EKey::Numpad7, VK_NUMPAD7 },
		{ EKey::Numpad8, VK_NUMPAD8 }, { EKey::Numpad9, VK_NUMPAD9 },

		{ EKey::Hangul, VK_HANGUL }, { EKey::Hanja, VK_HANJA },
		{ EKey::OEM102, VK_OEM_102 },
		{ EKey::BrowserBack, VK_BROWSER_BACK },
		{ EKey::BrowserForward, VK_BROWSER_FORWARD },
		{ EKey::BrowserRefresh, VK_BROWSER_REFRESH },
		{ EKey::BrowserStop, VK_BROWSER_STOP },
		{ EKey::BrowserSearch, VK_BROWSER_SEARCH },
		{ EKey::BrowserFavorites, VK_BROWSER_FAVORITES },
		{ EKey::BrowserHome, VK_BROWSER_HOME },
		{ EKey::VolumeMute, VK_VOLUME_MUTE },
		{ EKey::VolumeDown, VK_VOLUME_DOWN }, { EKey::VolumeUp, VK_VOLUME_UP },
		{ EKey::MediaNextTrack, VK_MEDIA_NEXT_TRACK },
		{ EKey::MediaPreviousTrack, VK_MEDIA_PREV_TRACK },
		{ EKey::MediaStop, VK_MEDIA_STOP },
		{ EKey::MediaPlayPause, VK_MEDIA_PLAY_PAUSE },
		{ EKey::LaunchMail, VK_LAUNCH_MAIL },
		{ EKey::LaunchMediaSelect, VK_LAUNCH_MEDIA_SELECT },
		{ EKey::LaunchApplication1, VK_LAUNCH_APP1 },
		{ EKey::LaunchApplication2, VK_LAUNCH_APP2 }
	};

	static_assert(136 == std::size(PolledKeys), "The Win32 keyboard table changed.");

}

EKey InputSubsystem::GetKeyFromVirtualKey(uint32_t _VirtualKey)
{
	for (const FPolledKey& PolledKey : PolledKeys)
	{
		if (PolledKey.__VirtualKey == static_cast<int>(_VirtualKey))
			return PolledKey.__Key;
	}

	switch (_VirtualKey)
	{
	case VK_LBUTTON: return EKey::LeftMouseButton;
	case VK_RBUTTON: return EKey::RightMouseButton;
	case VK_MBUTTON: return EKey::MiddleMouseButton;
	case VK_XBUTTON1: return EKey::ThumbMouseButton;
	case VK_XBUTTON2: return EKey::ThumbMouseButton2;
	default: return EKey::Invalid;
	}
}

InputSubsystem::InputSubsystem() = default;
InputSubsystem::~InputSubsystem() = default;

HRESULT InputSubsystem::Initialize()
{
	__KeyStates.clear();
	for (const FPolledKey& PolledKey : PolledKeys)
		__KeyStates.emplace(PolledKey.__Key, false);

	return S_OK;
}

bool InputSubsystem::InitializePlatformApplication(HWND _WindowHandle)
{
	if (__PlatformApplication)
		return true;

	__PlatformApplication = std::make_unique<FWindowsInputApplication>();
	if (__PlatformApplication->Initialize(_WindowHandle))
		return true;

	__PlatformApplication.reset();
	return false;
}

void InputSubsystem::Deinitialize()
{
	std::lock_guard<std::mutex> Lock(__PlayerControllerMutex);
	__PlayerController = nullptr;
	__KeyStates.clear();
	if (__PlatformApplication)
		__PlatformApplication->Deinitialize();
	__PlatformApplication.reset();
}

void InputSubsystem::Tick(float _DeltaTime)
{
	TickInput(_DeltaTime);
}

void InputSubsystem::TickInput(float _DeltaTime)
{
	APlayerController* PlayerController = nullptr;
	{
		std::lock_guard<std::mutex> Lock(__PlayerControllerMutex);
		PlayerController = __PlayerController;
	}

	if (nullptr == PlayerController)
	{
		for (auto& KeyState : __KeyStates)
			KeyState.second = false;
		return;
	}

	std::vector<FWindowsInputEvent> PlatformEvents;
	if (__PlatformApplication)
		__PlatformApplication->DrainEvents(PlatformEvents);

	std::vector<FKeyInputEvent> InputEvents;
	std::vector<FAnalogInputEvent> AnalogEvents;
	for (const FWindowsInputEvent& PlatformEvent : PlatformEvents)
	{
		switch (PlatformEvent.__Type)
		{
		case EWindowsInputEventType::KeyDown:
		case EWindowsInputEventType::MouseButtonDown:
			{
				const EKey Key = GetKeyFromVirtualKey(PlatformEvent.__Code);
				if (EKey::Invalid != Key)
				{
					__KeyStates[Key] = true;
					InputEvents.push_back({ Key, PlatformEvent.__bRepeat ?
						EInputEvent::Repeat : EInputEvent::Pressed });
				}
			}
			break;
		case EWindowsInputEventType::KeyUp:
		case EWindowsInputEventType::MouseButtonUp:
			{
				const EKey Key = GetKeyFromVirtualKey(PlatformEvent.__Code);
				if (EKey::Invalid != Key)
				{
					__KeyStates[Key] = false;
					InputEvents.push_back({ Key, EInputEvent::Released });
				}
			}
			break;
		case EWindowsInputEventType::MouseWheel:
			AnalogEvents.push_back({ EKey::MouseWheelAxis, PlatformEvent.__ValueX });
			break;
		case EWindowsInputEventType::RawMouseMove:
			AnalogEvents.push_back({ EKey::MouseX, PlatformEvent.__ValueX });
			AnalogEvents.push_back({ EKey::MouseY, PlatformEvent.__ValueY });
			break;
		case EWindowsInputEventType::FocusLost:
			for (auto& KeyState : __KeyStates)
			{
				if (KeyState.second)
					InputEvents.push_back({ KeyState.first, EInputEvent::Released });
				KeyState.second = false;
			}
			break;
		}
	}

	PlayerController->TickPlayerInput(InputEvents, AnalogEvents, _DeltaTime, false);
}

void InputSubsystem::SetPlayerController(APlayerController* _PlayerController)
{
	std::lock_guard<std::mutex> Lock(__PlayerControllerMutex);
	__PlayerController = _PlayerController;
}

void InputSubsystem::ClearPlayerController(APlayerController* _PlayerController)
{
	std::lock_guard<std::mutex> Lock(__PlayerControllerMutex);
	if (__PlayerController == _PlayerController)
		__PlayerController = nullptr;
}
