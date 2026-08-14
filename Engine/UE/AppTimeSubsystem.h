#pragma once

#include "Subsystem.h"

#include <Windows.h>
#include <memory>

class FAppTimeRenderProxy;

class AppTimeSubsystem : public ISubsystem
{
public:
	AppTimeSubsystem() = default;
	virtual ~AppTimeSubsystem() override = default;

	virtual HRESULT Initialize() override;
	virtual void Deinitialize() override;
	virtual void Tick(float _DeltaTime) override;

	double GetCurrentTime() const { return __CurrentTime; }
	double GetDeltaTime() const { return __DeltaTime; }
	double GetRawDeltaTime() const { return __RawDeltaTime; }
	double GetFramesPerSecond() const { return __FramesPerSecond; }

private:
	double MeasureDeltaTime();

	void UpdateFramesPerSecond();

	void PublishRenderData() const;

private:
	LARGE_INTEGER __Frequency = {};

	LARGE_INTEGER __PreviousCounter = {};

	double __DeltaTime = 0.0;

	double __RawDeltaTime = 0.0;

	double __CurrentTime = 0.0;

	double __AverageFrameTime = 0.0;
	double __FramesPerSecond = 0.0;

	std::shared_ptr<FAppTimeRenderProxy> __RenderProxy;
};
