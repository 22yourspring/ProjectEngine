#pragma once

#include "SceneViewExtension.h"

#include <mutex>
#include <string>

class FAppTimeRenderProxy final : public ISceneViewExtension
{
public:
	void Submit(const char* _ClockName, double _FramesPerSecond, double _CurrentTime);
	virtual void Render(FDynamicRHI& _DynamicRHI) override;

private:
	struct FSnapshot
	{
		std::string ClockName;
		double FramesPerSecond = 0.0;
		double CurrentTime = 0.0;
	};

	std::mutex __SnapshotMutex;
	FSnapshot __PendingSnapshot;
	bool __bHasPendingSnapshot = false;
};
