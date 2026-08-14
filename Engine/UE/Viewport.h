#pragma once

#include "DynamicRHI.h"

class FViewportClient;

class FViewport
{
public:
	explicit FViewport(FViewportClient* _Client);
	virtual ~FViewport() = default;

	FViewportClient* GetClient() const { return __Client; }
	FRHIViewport* GetViewportRHI() const { return __ViewportRHI.get(); }
	const FViewportRHIRef& GetViewportRHIRef() const { return __ViewportRHI; }
	uint32 GetSizeX() const { return __SizeX; }
	uint32 GetSizeY() const { return __SizeY; }

protected:
	void SetViewportRHI(FViewportRHIRef _ViewportRHI) { __ViewportRHI = std::move(_ViewportRHI); }
	void SetSize(uint32 _SizeX, uint32 _SizeY) { __SizeX = _SizeX; __SizeY = _SizeY; }

private:
	FViewportClient* __Client = nullptr;
	FViewportRHIRef __ViewportRHI;
	uint32 __SizeX = 0;
	uint32 __SizeY = 0;
};

class FSceneViewport final : public FViewport
{
public:
	FSceneViewport(FViewportClient* _Client, void* _WindowHandle);

	bool Initialize(FDynamicRHI& _DynamicRHI, uint32 _SizeX, uint32 _SizeY);
	bool ResizeFrame(FDynamicRHI& _DynamicRHI, uint32 _SizeX, uint32 _SizeY);
	void ReleaseRHI();
	void* GetWindowHandle() const { return __WindowHandle; }

private:
	void* __WindowHandle = nullptr;
};
