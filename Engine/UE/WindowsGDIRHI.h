#pragma once

#include "DynamicRHI.h"
#include "ThirdParty/WIL/include/wil/resource.h"

class FWindowsGDIViewport final : public FRHIViewport
{
public:
	FWindowsGDIViewport() = default;
	virtual ~FWindowsGDIViewport() override;

	bool Initialize(const FRHIViewportDesc& _Desc);
	bool Resize(uint32 _SizeX, uint32 _SizeY);
	void Release();
	bool Present();

	HWND GetWindowHandle() const { return __WindowHandle; }
	HDC GetBackBufferDC() const { return __BackBufferDC.get(); }
	uint32 GetSizeX() const { return __SizeX; }
	uint32 GetSizeY() const { return __SizeY; }

private:
	HWND __WindowHandle = nullptr;
	wil::unique_hdc __BackBufferDC;
	wil::unique_hbitmap __BackBufferBitmap;
	HGDIOBJ __PreviousBitmap = nullptr;
	uint32 __SizeX = 0;
	uint32 __SizeY = 0;
};

class FWindowsGDIRHI final : public FDynamicRHI
{
public:
	virtual ~FWindowsGDIRHI() override;

	virtual bool Init() override;
	virtual void Shutdown() override;

	virtual FViewportRHIRef RHICreateViewport(
		const FRHIViewportDesc& _Desc) override;

	virtual bool RHIResizeViewport(
		FRHIViewport* _Viewport,
		uint32 _SizeX,
		uint32 _SizeY) override;

	virtual bool RHIBeginDrawingViewport(
		FRHIViewport* _Viewport,
		const FColor& _ClearColor) override;

	virtual void RHIDrawLine(int32 _StartX, int32 _StartY, int32 _EndX, int32 _EndY,
		const FColor& _Color, int32 _Thickness = 1) override;

	virtual void RHIDrawRectangle(int32 _Left, int32 _Top, int32 _Right, int32 _Bottom,
		const FColor& _Color) override;

	virtual void RHIEndDrawingViewport(FRHIViewport* _Viewport, bool _bPresent) override;

private:
	FWindowsGDIViewport* __DrawingViewport = nullptr;
};
