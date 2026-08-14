#pragma once

class FDynamicRHI;

class ISceneViewExtension
{
public:
	virtual ~ISceneViewExtension() = default;
	virtual void Render(FDynamicRHI& _DynamicRHI) = 0;
};
