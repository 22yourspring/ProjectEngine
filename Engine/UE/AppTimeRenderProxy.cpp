#include "pch.h"
#include "AppTimeRenderProxy.h"
#include "LaunchEngineLoop.h"

void FAppTimeRenderProxy::Submit(
	const char* _ClockName,
	double _FramesPerSecond,
	double _CurrentTime)
{
	std::lock_guard<std::mutex> Lock(__SnapshotMutex);

	__PendingSnapshot.ClockName = nullptr != _ClockName ? _ClockName : "Unknown";
	__PendingSnapshot.FramesPerSecond = _FramesPerSecond;
	__PendingSnapshot.CurrentTime = _CurrentTime;
	__bHasPendingSnapshot = true;
}

void FAppTimeRenderProxy::Render(FDynamicRHI& _DynamicRHI)
{
#ifdef _DEBUG
	UNREFERENCED_PARAMETER(_DynamicRHI);

	FSnapshot Snapshot;

	{
		std::lock_guard<std::mutex> Lock(__SnapshotMutex);

		if (false == __bHasPendingSnapshot)
			return;

		Snapshot = __PendingSnapshot;
		__bHasPendingSnapshot = false;
	}

	HWND WindowHandle = FEngineLoop::GetInstance()->GetHandle();
	if (nullptr == WindowHandle)
		return;

	char WindowTitle[112] = {};
	sprintf_s(
		WindowTitle,
		"%s Game FPS: %.1f  Play Time: %.2fs",
		Snapshot.ClockName.c_str(),
		Snapshot.FramesPerSecond,
		Snapshot.CurrentTime);

	SetWindowTextA(WindowHandle, WindowTitle);
#else
	UNREFERENCED_PARAMETER(_DynamicRHI);
#endif
}
