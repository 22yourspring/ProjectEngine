#pragma once

#include "UE/DynamicRHI.h"

#include <d3d11.h>
#include <dxgi.h>
#pragma push_macro("Super")
#undef Super
#include <wrl/client.h>
#pragma pop_macro("Super")

class FD3D11DynamicRHI;

class FD3D11Viewport final : public FRHIViewport
{
public:
	FD3D11Viewport() = default;
	virtual ~FD3D11Viewport() override = default;

	bool Initialize(
		FD3D11DynamicRHI& _D3DRHI,
		const FRHIViewportDesc& _Desc);
	bool Resize(uint32 _SizeX, uint32 _SizeY);
	bool Present();

	ID3D11RenderTargetView* GetRenderTargetView() const { return __RenderTargetView.Get(); }
	uint32 GetSizeX() const { return __SizeX; }
	uint32 GetSizeY() const { return __SizeY; }
	uint32 GetLogicalSizeX() const { return __LogicalSizeX; }
	uint32 GetLogicalSizeY() const { return __LogicalSizeY; }

private:
	bool CreateBackBuffer();

private:
	FD3D11DynamicRHI* __D3DRHI = nullptr;
	HWND __WindowHandle = nullptr;
	Microsoft::WRL::ComPtr<IDXGISwapChain> __SwapChain;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> __RenderTargetView;
	uint32 __SizeX = 0;
	uint32 __SizeY = 0;
	// Stable game-space resolution. __SizeX/__SizeY track the actual window.
	uint32 __LogicalSizeX = 0;
	uint32 __LogicalSizeY = 0;
	ERHIPresentMode __PresentMode = ERHIPresentMode::VSync;
};

class FD3D11DynamicRHI final : public FDynamicRHI
{
public:
	virtual ~FD3D11DynamicRHI() override;
	virtual const char* GetName() const override { return "D3D11"; }
	virtual ERHIInterfaceType GetInterfaceType() const override { return ERHIInterfaceType::D3D11; }

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
	virtual void RHIDrawLine(
		int32 _StartX,
		int32 _StartY,
		int32 _EndX,
		int32 _EndY,
		const FColor& _Color,
		int32 _Thickness = 1) override;
	virtual void RHIDrawRectangle(
		int32 _Left,
		int32 _Top,
		int32 _Right,
		int32 _Bottom,
		const FColor& _Color) override;
	virtual void RHIEndDrawingViewport(
		FRHIViewport* _Viewport,
		bool _bPresent) override;

	ID3D11Device* GetDevice() const { return __Device.Get(); }
	ID3D11DeviceContext* GetDeviceContext() const { return __DeviceContext.Get(); }
	IDXGIFactory* GetDXGIFactory() const { return __DXGIFactory.Get(); }

private:
	struct FD3D11SimpleVertex
	{
		float Position[2];
		float Color[4];
	};

	bool CreateRenderingPipeline();
	void DrawVertices(
		const FD3D11SimpleVertex* _Vertices,
		uint32 _VertexCount,
		D3D11_PRIMITIVE_TOPOLOGY _Topology);
	FD3D11SimpleVertex MakeVertex(float _X, float _Y, const FColor& _Color) const;

private:
	Microsoft::WRL::ComPtr<ID3D11Device> __Device;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> __DeviceContext;
	Microsoft::WRL::ComPtr<IDXGIFactory> __DXGIFactory;
	Microsoft::WRL::ComPtr<ID3D11VertexShader> __VertexShader;
	Microsoft::WRL::ComPtr<ID3D11PixelShader> __PixelShader;
	Microsoft::WRL::ComPtr<ID3D11InputLayout> __InputLayout;
	Microsoft::WRL::ComPtr<ID3D11Buffer> __DynamicVertexBuffer;
	Microsoft::WRL::ComPtr<ID3D11BlendState> __BlendState;
	Microsoft::WRL::ComPtr<ID3D11RasterizerState> __RasterizerState;
	FD3D11Viewport* __DrawingViewport = nullptr;
	D3D_FEATURE_LEVEL __FeatureLevel = D3D_FEATURE_LEVEL_9_1;
};

class FD3D11DynamicRHIModule final : public IDynamicRHIModule
{
public:
	virtual const char* GetName() const override { return "D3D11RHI"; }
	virtual bool IsSupported() const override;
	virtual std::unique_ptr<FDynamicRHI> CreateRHI() override;
};
