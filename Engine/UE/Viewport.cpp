#include "pch.h"
#include "Viewport.h"

FViewport::FViewport(FViewportClient* _Client)
	: __Client(_Client)
{
}

FSceneViewport::FSceneViewport(FViewportClient* _Client, void* _WindowHandle)
	: FViewport(_Client), __WindowHandle(_WindowHandle)
{
}

bool FSceneViewport::Initialize(
	FDynamicRHI& _DynamicRHI,
	uint32 _SizeX,
	uint32 _SizeY)
{
	FRHIViewportDesc ViewportDesc;
	ViewportDesc.WindowHandle = __WindowHandle;
	ViewportDesc.SizeX = _SizeX;
	ViewportDesc.SizeY = _SizeY;
	ViewportDesc.BufferCount = 2;
	ViewportDesc.bIsFullscreen = false;
	ViewportDesc.PresentMode = ERHIPresentMode::VSync;

	SetSize(_SizeX, _SizeY);
	SetViewportRHI(_DynamicRHI.RHICreateViewport(ViewportDesc));

	return nullptr != GetViewportRHI();
}

bool FSceneViewport::ResizeFrame(
	FDynamicRHI& _DynamicRHI,
	uint32 _SizeX,
	uint32 _SizeY)
{
	if (0 == _SizeX || 0 == _SizeY || nullptr == GetViewportRHI())
		return false;

	if (false == _DynamicRHI.RHIResizeViewport(
		GetViewportRHI(), _SizeX, _SizeY))
	{
		return false;
	}

	SetSize(_SizeX, _SizeY);
	return true;
}

void FSceneViewport::ReleaseRHI()
{
	SetViewportRHI(nullptr);
}
