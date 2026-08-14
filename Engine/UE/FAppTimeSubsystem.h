#pragma once

#include "Subsystem.h"

#include <chrono>
#include <memory>

class FAppTimeRenderProxy;

class FAppTimeSubsystem : public ISubsystem
{
private:
	using FClock = std::chrono::steady_clock;

public:
	FAppTimeSubsystem() = default;
	virtual ~FAppTimeSubsystem() override = default;

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
	FClock::time_point __PreviousTime = {};

	double __CurrentTime = 0.0;
	double __DeltaTime = 0.0;
	double __RawDeltaTime = 0.0;
	double __AverageFrameTime = 0.0;
	double __FramesPerSecond = 0.0;
	std::shared_ptr<FAppTimeRenderProxy> __RenderProxy;
};
