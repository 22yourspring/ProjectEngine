#include "pch.h"
#include "FAppTimeSubsystem.h"
#include "Engine.h"
#include "RenderSubsystem.h"
#include "AppTimeRenderProxy.h"

#include <algorithm>

HRESULT FAppTimeSubsystem::Initialize()
{
	__PreviousTime = FClock::now();
	__CurrentTime = 0.0;
	__DeltaTime = 0.0;
	__RawDeltaTime = 0.0;
	__AverageFrameTime = 0.0;
	__FramesPerSecond = 0.0;

	RenderSubsystem* Renderer = GEngine->GetSubsystem<RenderSubsystem>();
	if (nullptr == Renderer)
		return E_FAIL;

	__RenderProxy = std::make_shared<FAppTimeRenderProxy>();
	Renderer->RegisterSceneViewExtension(__RenderProxy);

	return S_OK;
}

void FAppTimeSubsystem::Deinitialize()
{
	__PreviousTime = {};
	__CurrentTime = 0.0;
	__DeltaTime = 0.0;
	__RawDeltaTime = 0.0;
	__AverageFrameTime = 0.0;
	__FramesPerSecond = 0.0;
	__RenderProxy.reset();
}

void FAppTimeSubsystem::Tick(float _DeltaTime)
{
	UNREFERENCED_PARAMETER(_DeltaTime);

	__RawDeltaTime = MeasureDeltaTime();
	__CurrentTime += __RawDeltaTime;

	constexpr double MaxDeltaTime = 0.1;
	__DeltaTime = std::clamp(__RawDeltaTime, 0.0, MaxDeltaTime);

	UpdateFramesPerSecond();
	PublishRenderData();
}

double FAppTimeSubsystem::MeasureDeltaTime()
{
	const FClock::time_point CurrentTime = FClock::now();
	const double DeltaTime =
		std::chrono::duration<double>(CurrentTime - __PreviousTime).count();

	__PreviousTime = CurrentTime;

	return DeltaTime;
}

void FAppTimeSubsystem::UpdateFramesPerSecond()
{
	if (__RawDeltaTime <= 0.0)
		return;

	if (__AverageFrameTime <= 0.0)
	{
		__AverageFrameTime = __RawDeltaTime;
	}
	else
	{
		__AverageFrameTime =
			__AverageFrameTime * 0.75 + __RawDeltaTime * 0.25;
	}

	__FramesPerSecond = 1.0 / __AverageFrameTime;
}

void FAppTimeSubsystem::PublishRenderData() const
{
	if (nullptr == __RenderProxy)
		return;

	__RenderProxy->Submit("Chrono", __FramesPerSecond, __CurrentTime);
}
