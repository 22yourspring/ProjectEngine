#include "pch.h"
#include "WindowsInputApplication.h"

namespace
{
	constexpr wchar_t InputApplicationProperty[] = L"EngineSession.WindowsInputApplication";
}

FWindowsInputApplication::~FWindowsInputApplication()
{
	Deinitialize();
}

bool FWindowsInputApplication::Initialize(HWND _WindowHandle)
{
	if (nullptr == _WindowHandle || false == IsWindow(_WindowHandle))
		return false;

	if (nullptr != __WindowHandle)
		return __WindowHandle == _WindowHandle;

	if (false == SetPropW(_WindowHandle, InputApplicationProperty, this))
		return false;

	SetLastError(ERROR_SUCCESS);
	const LONG_PTR PreviousWindowProc = SetWindowLongPtrW(
		_WindowHandle, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(&SubclassWindowProc));
	if (0 == PreviousWindowProc && ERROR_SUCCESS != GetLastError())
	{
		RemovePropW(_WindowHandle, InputApplicationProperty);
		return false;
	}

	__WindowHandle = _WindowHandle;
	__OriginalWindowProc = reinterpret_cast<WNDPROC>(PreviousWindowProc);

	RAWINPUTDEVICE MouseDevice = {};
	MouseDevice.usUsagePage = 0x01;
	MouseDevice.usUsage = 0x02;
	MouseDevice.dwFlags = 0;
	MouseDevice.hwndTarget = _WindowHandle;
	return TRUE == RegisterRawInputDevices(&MouseDevice, 1, sizeof(MouseDevice));
}

void FWindowsInputApplication::Deinitialize()
{
	if (nullptr != __WindowHandle && IsWindow(__WindowHandle) && nullptr != __OriginalWindowProc)
	{
		SetWindowLongPtrW(__WindowHandle, GWLP_WNDPROC,
			reinterpret_cast<LONG_PTR>(__OriginalWindowProc));
		RemovePropW(__WindowHandle, InputApplicationProperty);
	}

	__WindowHandle = nullptr;
	__OriginalWindowProc = nullptr;

	std::lock_guard<std::mutex> Lock(__EventMutex);
	__PendingEvents.clear();
}

void FWindowsInputApplication::DrainEvents(std::vector<FWindowsInputEvent>& _OutEvents)
{
	std::lock_guard<std::mutex> Lock(__EventMutex);
	_OutEvents.clear();
	_OutEvents.swap(__PendingEvents);
}

LRESULT CALLBACK FWindowsInputApplication::SubclassWindowProc(
	HWND _Hwnd, UINT _Message, WPARAM _WParam, LPARAM _LParam)
{
	auto* Application = reinterpret_cast<FWindowsInputApplication*>(
		GetPropW(_Hwnd, InputApplicationProperty));
	if (nullptr == Application || nullptr == Application->__OriginalWindowProc)
		return DefWindowProcW(_Hwnd, _Message, _WParam, _LParam);

	return Application->ProcessMessage(_Hwnd, _Message, _WParam, _LParam);
}

LRESULT FWindowsInputApplication::ProcessMessage(
	HWND _Hwnd, UINT _Message, WPARAM _WParam, LPARAM _LParam)
{
	switch (_Message)
	{
	case WM_KEYDOWN:
	case WM_SYSKEYDOWN:
		EnqueueEvent({ EWindowsInputEventType::KeyDown, static_cast<uint32_t>(_WParam),
			0.0f, 0.0f, 0 != (_LParam & (1LL << 30)) });
		break;
	case WM_KEYUP:
	case WM_SYSKEYUP:
		EnqueueEvent({ EWindowsInputEventType::KeyUp, static_cast<uint32_t>(_WParam) });
		break;
	case WM_LBUTTONDOWN: EnqueueEvent({ EWindowsInputEventType::MouseButtonDown, VK_LBUTTON }); break;
	case WM_LBUTTONUP: EnqueueEvent({ EWindowsInputEventType::MouseButtonUp, VK_LBUTTON }); break;
	case WM_RBUTTONDOWN: EnqueueEvent({ EWindowsInputEventType::MouseButtonDown, VK_RBUTTON }); break;
	case WM_RBUTTONUP: EnqueueEvent({ EWindowsInputEventType::MouseButtonUp, VK_RBUTTON }); break;
	case WM_MBUTTONDOWN: EnqueueEvent({ EWindowsInputEventType::MouseButtonDown, VK_MBUTTON }); break;
	case WM_MBUTTONUP: EnqueueEvent({ EWindowsInputEventType::MouseButtonUp, VK_MBUTTON }); break;
	case WM_XBUTTONDOWN:
	case WM_XBUTTONUP:
		EnqueueEvent({ WM_XBUTTONDOWN == _Message ? EWindowsInputEventType::MouseButtonDown :
			EWindowsInputEventType::MouseButtonUp,
			static_cast<uint32_t>(XBUTTON1 == GET_XBUTTON_WPARAM(_WParam) ? VK_XBUTTON1 : VK_XBUTTON2) });
		break;
	case WM_MOUSEWHEEL:
		EnqueueEvent({ EWindowsInputEventType::MouseWheel, 0,
			static_cast<float>(GET_WHEEL_DELTA_WPARAM(_WParam)) / WHEEL_DELTA });
		break;
	case WM_INPUT:
		{
			RAWINPUT RawInput = {};
			UINT Size = sizeof(RawInput);
			if (sizeof(RawInput) == GetRawInputData(reinterpret_cast<HRAWINPUT>(_LParam), RID_INPUT,
				&RawInput, &Size, sizeof(RAWINPUTHEADER)) && RIM_TYPEMOUSE == RawInput.header.dwType)
			{
				EnqueueEvent({ EWindowsInputEventType::RawMouseMove, 0,
					static_cast<float>(RawInput.data.mouse.lLastX),
					static_cast<float>(RawInput.data.mouse.lLastY) });
			}
		}
		break;
	case WM_KILLFOCUS:
		EnqueueEvent({ EWindowsInputEventType::FocusLost });
		break;
	default:
		break;
	}

	const LRESULT Result = CallWindowProcW(__OriginalWindowProc, _Hwnd, _Message, _WParam, _LParam);
	if (WM_NCDESTROY == _Message)
	{
		RemovePropW(_Hwnd, InputApplicationProperty);
		__WindowHandle = nullptr;
	}
	return Result;
}

void FWindowsInputApplication::EnqueueEvent(const FWindowsInputEvent& _Event)
{
	std::lock_guard<std::mutex> Lock(__EventMutex);
	__PendingEvents.push_back(_Event);
}
