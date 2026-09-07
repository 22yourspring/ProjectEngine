#pragma once

#include "EngineSystem.h"

#include <Windows.h>
#include <memory>

class FAppTimeRenderProxy;

UCLASS()
class ENGINE_API AppTimeEngineSystem : public IEngineSystem
{
	GENERATED_BODY()

public:
	AppTimeEngineSystem() = default;
	virtual ~AppTimeEngineSystem() override = default;

	virtual HRESULT Initialize() override;
	virtual void Deinitialize() override;
	virtual void Tick(float _DeltaTime) override;
	virtual bool UsesTickGroup() const override { return true; }

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
