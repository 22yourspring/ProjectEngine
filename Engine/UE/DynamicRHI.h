#pragma once

#include "EngineSystem.h"
#include "CoreTypes.h"
#include "Math/Color.h"

class FRHIViewport
{
public:
	virtual ~FRHIViewport() = default;
};

using FViewportRHIRef = std::shared_ptr<FRHIViewport>;

enum class ERHIPresentMode
{
	Immediate,
	VSync
};

struct FRHIViewportDesc
{
	void* WindowHandle = nullptr;
	uint32 SizeX = 0;
	uint32 SizeY = 0;
	uint32 BufferCount = 2;
	bool bIsFullscreen = false;
	ERHIPresentMode PresentMode = ERHIPresentMode::VSync;
};

class FDynamicRHI
{
public:
	virtual ~FDynamicRHI() = default;

	virtual bool Init() = 0;

	virtual void Shutdown() = 0;

	virtual FViewportRHIRef RHICreateViewport(
		const FRHIViewportDesc& _Desc) = 0;

	virtual bool RHIResizeViewport(
		FRHIViewport* _Viewport,
		uint32 _SizeX,
		uint32 _SizeY) = 0;

	virtual bool RHIBeginDrawingViewport(
		FRHIViewport* _Viewport,
		const FColor& _ClearColor) = 0;

	virtual void RHIDrawLine(int32 _StartX, int32 _StartY, int32 _EndX, int32 _EndY,
		const FColor& _Color, int32 _Thickness = 1) = 0;

	virtual void RHIDrawRectangle(int32 _Left, int32 _Top, int32 _Right, int32 _Bottom,
		const FColor& _Color) = 0;

	virtual void RHIEndDrawingViewport(FRHIViewport* _Viewport, bool _bPresent) = 0;
};

std::unique_ptr<FDynamicRHI> PlatformCreateDynamicRHI();
