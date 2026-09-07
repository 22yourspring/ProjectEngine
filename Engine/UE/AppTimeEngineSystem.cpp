#include "pch.h"
#include "AppTimeEngineSystem.h"
#include "Engine.h"
#include "LaunchEngineLoop.h"
#include "RenderEngineSystem.h"
#include "AppTimeRenderProxy.h"

#include <algorithm>

HRESULT AppTimeEngineSystem::Initialize()
{
	PrimaryEngineSystemTick.TickGroup = ETickingGroup::TG_PostUpdateWork;

	if (!QueryPerformanceFrequency(&__Frequency))
		return HRESULT_FROM_WIN32(GetLastError());

	if (!QueryPerformanceCounter(&__PreviousCounter))
		return HRESULT_FROM_WIN32(GetLastError());

	__DeltaTime = 0.0;
	__RawDeltaTime = 0.0;
	__CurrentTime = 0.0;
	__AverageFrameTime = 0.0;
	__FramesPerSecond = 0.0;

	RenderEngineSystem* Renderer = GEngine->GetEngineSystem<RenderEngineSystem>();
	if (nullptr == Renderer)
		return E_FAIL;

	__RenderProxy = std::make_shared<FAppTimeRenderProxy>();
	Renderer->RegisterSceneViewExtension(__RenderProxy);

	return S_OK;
}

void AppTimeEngineSystem::Deinitialize()
{
	__Frequency = {};
	__PreviousCounter = {};
	__DeltaTime = 0.0;
	__RawDeltaTime = 0.0;
	__CurrentTime = 0.0;
	__AverageFrameTime = 0.0;
	__FramesPerSecond = 0.0;
	__RenderProxy.reset();
}

void AppTimeEngineSystem::Tick(float _DeltaTime)
{
	UNREFERENCED_PARAMETER(_DeltaTime);

	__RawDeltaTime = MeasureDeltaTime();
	__CurrentTime += __RawDeltaTime;

	constexpr double MaxDeltaTime = 0.1;
	__DeltaTime = std::clamp(__RawDeltaTime, 0.0, MaxDeltaTime);

	UpdateFramesPerSecond();
	PublishRenderData();

	FEngineLoop::GetInstance()->SetDeltaTime(static_cast<float>(__DeltaTime));
}

double AppTimeEngineSystem::MeasureDeltaTime()
{
	LARGE_INTEGER CurrentCounter = {};

	if (!QueryPerformanceCounter(&CurrentCounter))
		return 0.0;

	const double DeltaTime =
		static_cast<double>(CurrentCounter.QuadPart - __PreviousCounter.QuadPart) /
		static_cast<double>(__Frequency.QuadPart);

	__PreviousCounter = CurrentCounter;

	return DeltaTime;
}

void AppTimeEngineSystem::UpdateFramesPerSecond()
{
	if (__RawDeltaTime <= 0.0)
		return;

	if (__AverageFrameTime <= 0.0)
	{
		__AverageFrameTime = __RawDeltaTime;
	}
	else
	{
		__AverageFrameTime = __AverageFrameTime * 0.75 + __RawDeltaTime * 0.25;
	}

	__FramesPerSecond = 1.0 / __AverageFrameTime;
}

void AppTimeEngineSystem::PublishRenderData() const
{
	if (nullptr == __RenderProxy)
		return;

	__RenderProxy->Submit("QPC", __FramesPerSecond, __CurrentTime);
}
