#pragma once

#include <mutex>
#include <vector>

enum class EWindowsInputEventType : uint8_t
{
	KeyDown,
	KeyUp,
	MouseButtonDown,
	MouseButtonUp,
	MouseWheel,
	RawMouseMove,
	FocusLost
};

struct FWindowsInputEvent
{
	EWindowsInputEventType __Type = EWindowsInputEventType::KeyDown;
	uint32_t __Code = 0;
	float __ValueX = 0.0f;
	float __ValueY = 0.0f;
	bool __bRepeat = false;
};

class FWindowsInputApplication
{
public:
	FWindowsInputApplication() = default;
	~FWindowsInputApplication();

	bool Initialize(HWND _WindowHandle);
	void Deinitialize();
	void DrainEvents(std::vector<FWindowsInputEvent>& _OutEvents);

private:
	static LRESULT CALLBACK SubclassWindowProc(HWND _Hwnd, UINT _Message, WPARAM _WParam, LPARAM _LParam);
	LRESULT ProcessMessage(HWND _Hwnd, UINT _Message, WPARAM _WParam, LPARAM _LParam);
	void EnqueueEvent(const FWindowsInputEvent& _Event);

private:
	HWND __WindowHandle = nullptr;
	WNDPROC __OriginalWindowProc = nullptr;
	std::mutex __EventMutex;
	std::vector<FWindowsInputEvent> __PendingEvents;
};
