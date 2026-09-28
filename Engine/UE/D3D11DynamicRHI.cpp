#include "pch.h"
#include "D3D11DynamicRHI.h"

#include <cmath>
#include <cstring>
#include <d3dcompiler.h>
#include <iterator>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

using Microsoft::WRL::ComPtr;

namespace
{
	class FD3D11Texture final : public FRHITexture
	{
	public:
		ComPtr<ID3D11ShaderResourceView> __View;
	};

	constexpr D3D_FEATURE_LEVEL RequestedFeatureLevels[] =
	{
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0
	};

	HRESULT CreateD3D11Device(
		UINT _Flags,
		ID3D11Device** _Device,
		D3D_FEATURE_LEVEL* _FeatureLevel,
		ID3D11DeviceContext** _DeviceContext)
	{
		HRESULT Result = D3D11CreateDevice(
			nullptr,
			D3D_DRIVER_TYPE_HARDWARE,
			nullptr,
			_Flags,
			RequestedFeatureLevels,
			static_cast<UINT>(std::size(RequestedFeatureLevels)),
			D3D11_SDK_VERSION,
			_Device,
			_FeatureLevel,
			_DeviceContext);

		if (E_INVALIDARG == Result)
		{
			Result = D3D11CreateDevice(
				nullptr,
				D3D_DRIVER_TYPE_HARDWARE,
				nullptr,
				_Flags,
				RequestedFeatureLevels + 1,
				static_cast<UINT>(std::size(RequestedFeatureLevels) - 1),
				D3D11_SDK_VERSION,
				_Device,
				_FeatureLevel,
				_DeviceContext);
		}

		return Result;
	}
}

bool FD3D11Viewport::Initialize(
	FD3D11DynamicRHI& _D3DRHI,
	const FRHIViewportDesc& _Desc)
{
	HWND WindowHandle = static_cast<HWND>(_Desc.WindowHandle);
	if (nullptr == WindowHandle || FALSE == IsWindow(WindowHandle) ||
		0 == _Desc.SizeX || 0 == _Desc.SizeY ||
		nullptr == _D3DRHI.GetDXGIFactory() || nullptr == _D3DRHI.GetDevice())
	{
		return false;
	}

	DXGI_SWAP_CHAIN_DESC SwapChainDesc = {};
	SwapChainDesc.BufferDesc.Width = _Desc.SizeX;
	SwapChainDesc.BufferDesc.Height = _Desc.SizeY;
	SwapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	SwapChainDesc.SampleDesc.Count = 1;
	SwapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	SwapChainDesc.BufferCount = _Desc.BufferCount < 2 ? 2 : _Desc.BufferCount;
	SwapChainDesc.OutputWindow = WindowHandle;
	SwapChainDesc.Windowed = _Desc.bIsFullscreen ? FALSE : TRUE;
	SwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
	SwapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	if (FAILED(_D3DRHI.GetDXGIFactory()->CreateSwapChain(
		_D3DRHI.GetDevice(), &SwapChainDesc, __SwapChain.GetAddressOf())))
	{
		return false;
	}

	_D3DRHI.GetDXGIFactory()->MakeWindowAssociation(
		WindowHandle,
		DXGI_MWA_NO_ALT_ENTER | DXGI_MWA_NO_WINDOW_CHANGES);

	__D3DRHI = &_D3DRHI;
	__WindowHandle = WindowHandle;
	__SizeX = _Desc.SizeX;
	__SizeY = _Desc.SizeY;
	__LogicalSizeX = _Desc.SizeX;
	__LogicalSizeY = _Desc.SizeY;
	__PresentMode = _Desc.PresentMode;
	return CreateBackBuffer();
}

FTextureRHIRef FD3D11DynamicRHI::RHICreateTexture2D(uint32 _Width, uint32 _Height, const std::vector<uint8>& _Pixels)
{
    if (!__Device || !_Width || !_Height || _Width > 8192 || _Height > 8192 || _Pixels.size() != uint64(_Width) * _Height * 4) return nullptr;
    D3D11_TEXTURE2D_DESC Desc = {};
    Desc.Width = _Width;
    Desc.Height = _Height;
    Desc.MipLevels = Desc.ArraySize = 1;
    Desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    Desc.SampleDesc.Count = 1;
    Desc.Usage = D3D11_USAGE_IMMUTABLE;
    Desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    D3D11_SUBRESOURCE_DATA Data = { _Pixels.data(), _Width * 4, 0 };
    ComPtr<ID3D11Texture2D> Texture;
    auto Resource = std::make_shared<FD3D11Texture>();
    if (FAILED(__Device->CreateTexture2D(&Desc, &Data, Texture.GetAddressOf())) ||
        FAILED(__Device->CreateShaderResourceView(Texture.Get(), nullptr, Resource->__View.GetAddressOf()))) return nullptr;
    return Resource;
}

void FD3D11DynamicRHI::RHIDrawTexture(FRHITexture* _Texture, int32 _X, int32 _Y, int32 _Width, int32 _Height)
{
    const auto* Texture = dynamic_cast<FD3D11Texture*>(_Texture);
    if (!__DrawingViewport || !Texture || _Width <= 0 || _Height <= 0) return;
    const FColor White = { 255, 255, 255, 255 };
    const float Left = static_cast<float>(_X), Top = static_cast<float>(_Y);
    const float Right = Left + _Width, Bottom = Top + _Height;
    FD3D11SimpleVertex Vertices[] =
    {
        MakeVertex(Left, Top, White), MakeVertex(Right, Top, White), MakeVertex(Right, Bottom, White),
        MakeVertex(Left, Top, White), MakeVertex(Right, Bottom, White), MakeVertex(Left, Bottom, White)
    };
    Vertices[1].UV[0] = Vertices[2].UV[0] = Vertices[4].UV[0] = 1.0f;
    Vertices[2].UV[1] = Vertices[4].UV[1] = Vertices[5].UV[1] = 1.0f;
    DrawVertices(Vertices, 6, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST, Texture->__View.Get());
}

void FD3D11DynamicRHI::RHIDrawQuad(const FVector* _Corners, const FColor& _Color, FRHITexture* _Texture)
{
    if (!__DrawingViewport || !_Corners) return;
    const auto* Texture = dynamic_cast<FD3D11Texture*>(_Texture);
    FD3D11SimpleVertex Vertices[6];
    const int Indices[] = {0, 1, 2, 0, 2, 3};
    for (int Index = 0; Index < 6; ++Index)
    {
        const int Corner = Indices[Index];
        Vertices[Index] = MakeVertex(float(_Corners[Corner].X), float(_Corners[Corner].Y), _Color);
        Vertices[Index].UV[0] = Corner == 1 || Corner == 2 ? 1.f : 0.f;
        Vertices[Index].UV[1] = Corner >= 2 ? 1.f : 0.f;
    }
    DrawVertices(Vertices, 6, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST, Texture ? Texture->__View.Get() : nullptr);
}

bool FD3D11Viewport::CreateBackBuffer()
{
	if (nullptr == __D3DRHI || nullptr == __SwapChain)
		return false;

	ComPtr<ID3D11Texture2D> BackBuffer;
	if (FAILED(__SwapChain->GetBuffer(0, IID_PPV_ARGS(BackBuffer.GetAddressOf()))))
		return false;

	return SUCCEEDED(__D3DRHI->GetDevice()->CreateRenderTargetView(
		BackBuffer.Get(), nullptr, __RenderTargetView.GetAddressOf()));
}

bool FD3D11Viewport::Resize(uint32 _SizeX, uint32 _SizeY)
{
	if (nullptr == __SwapChain || 0 == _SizeX || 0 == _SizeY)
		return false;

	__RenderTargetView.Reset();
	if (FAILED(__SwapChain->ResizeBuffers(
		0,
		_SizeX,
		_SizeY,
		DXGI_FORMAT_UNKNOWN,
		DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH)))
	{
		return false;
	}

	__SizeX = _SizeX;
	__SizeY = _SizeY;
	return CreateBackBuffer();
}

bool FD3D11Viewport::Present()
{
	if (nullptr == __SwapChain)
		return false;

	const UINT SyncInterval = ERHIPresentMode::VSync == __PresentMode ? 1u : 0u;
	return SUCCEEDED(__SwapChain->Present(SyncInterval, 0));
}

FD3D11DynamicRHI::~FD3D11DynamicRHI()
{
	Shutdown();
}

bool FD3D11DynamicRHI::Init()
{
	if (nullptr != __Device)
		return true;

	UINT DeviceFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#if defined(_DEBUG)
	DeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	HRESULT Result = CreateD3D11Device(
		DeviceFlags,
		__Device.GetAddressOf(),
		&__FeatureLevel,
		__DeviceContext.GetAddressOf());

#if defined(_DEBUG)
	if (FAILED(Result))
	{
		DeviceFlags &= ~D3D11_CREATE_DEVICE_DEBUG;
		Result = CreateD3D11Device(
			DeviceFlags,
			__Device.GetAddressOf(),
			&__FeatureLevel,
			__DeviceContext.GetAddressOf());
	}
#endif

	if (FAILED(Result))
		return false;

	ComPtr<IDXGIDevice> DXGIDevice;
	ComPtr<IDXGIAdapter> Adapter;
	if (FAILED(__Device.As(&DXGIDevice)) ||
		FAILED(DXGIDevice->GetAdapter(Adapter.GetAddressOf())) ||
		FAILED(Adapter->GetParent(IID_PPV_ARGS(__DXGIFactory.GetAddressOf()))))
	{
		Shutdown();
		return false;
	}

	if (false == CreateRenderingPipeline())
	{
		Shutdown();
		return false;
	}

	return true;
}

void FD3D11DynamicRHI::Shutdown()
{
	__DrawingViewport = nullptr;
	if (__DeviceContext)
	{
		__DeviceContext->ClearState();
		__DeviceContext->Flush();
	}

	__RasterizerState.Reset();
	__WhiteTexture.Reset();
	__TextureSampler.Reset();
	__BlendState.Reset();
	__DynamicVertexBuffer.Reset();
	__InputLayout.Reset();
	__PixelShader.Reset();
	__VertexShader.Reset();
	__DXGIFactory.Reset();
	__DeviceContext.Reset();
	__Device.Reset();
}

bool FD3D11DynamicRHI::CreateRenderingPipeline()
{
	static constexpr char ShaderSource[] = R"(
struct VSInput
{
    float2 Position : POSITION;
    float4 Color : COLOR0;
    float2 UV : TEXCOORD0;
};

struct PSInput
{
    float4 Position : SV_POSITION;
    float4 Color : COLOR0;
    float2 UV : TEXCOORD0;
};

PSInput VSMain(VSInput Input)
{
    PSInput Output;
    Output.Position = float4(Input.Position, 0.0f, 1.0f);
    Output.Color = Input.Color;
    Output.UV = Input.UV;
    return Output;
}

Texture2D AssetTexture : register(t0);
SamplerState AssetSampler : register(s0);

float4 PSMain(PSInput Input) : SV_TARGET
{
    return Input.Color * AssetTexture.Sample(AssetSampler, Input.UV);
}
)";

	ComPtr<ID3DBlob> VertexShaderBlob;
	ComPtr<ID3DBlob> PixelShaderBlob;
	ComPtr<ID3DBlob> ErrorBlob;
	if (FAILED(D3DCompile(
		ShaderSource,
		sizeof(ShaderSource),
		"UnrealEngineSimpleRHI",
		nullptr,
		nullptr,
		"VSMain",
		"vs_4_0",
		D3DCOMPILE_ENABLE_STRICTNESS,
		0,
		VertexShaderBlob.GetAddressOf(),
		ErrorBlob.GetAddressOf())))
	{
		return false;
	}

	ErrorBlob.Reset();
	if (FAILED(D3DCompile(
		ShaderSource,
		sizeof(ShaderSource),
		"UnrealEngineSimpleRHI",
		nullptr,
		nullptr,
		"PSMain",
		"ps_4_0",
		D3DCOMPILE_ENABLE_STRICTNESS,
		0,
		PixelShaderBlob.GetAddressOf(),
		ErrorBlob.GetAddressOf())))
	{
		return false;
	}

	if (FAILED(__Device->CreateVertexShader(
		VertexShaderBlob->GetBufferPointer(),
		VertexShaderBlob->GetBufferSize(),
		nullptr,
		__VertexShader.GetAddressOf())) ||
		FAILED(__Device->CreatePixelShader(
			PixelShaderBlob->GetBufferPointer(),
			PixelShaderBlob->GetBufferSize(),
			nullptr,
			__PixelShader.GetAddressOf())))
	{
		return false;
	}

	constexpr D3D11_INPUT_ELEMENT_DESC InputElements[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 8, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};
	if (FAILED(__Device->CreateInputLayout(
		InputElements,
		static_cast<UINT>(std::size(InputElements)),
		VertexShaderBlob->GetBufferPointer(),
		VertexShaderBlob->GetBufferSize(),
		__InputLayout.GetAddressOf())))
	{
		return false;
	}

	D3D11_BUFFER_DESC VertexBufferDesc = {};
	VertexBufferDesc.ByteWidth = sizeof(FD3D11SimpleVertex) * 6;
	VertexBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	VertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	VertexBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	if (FAILED(__Device->CreateBuffer(
		&VertexBufferDesc,
		nullptr,
		__DynamicVertexBuffer.GetAddressOf())))
	{
		return false;
	}

	D3D11_BLEND_DESC BlendDesc = {};
	BlendDesc.RenderTarget[0].BlendEnable = TRUE;
	BlendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	BlendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	BlendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	BlendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	BlendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
	BlendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	BlendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
	if (FAILED(__Device->CreateBlendState(&BlendDesc, __BlendState.GetAddressOf())))
		return false;

	D3D11_RASTERIZER_DESC RasterizerDesc = {};
	const auto White = RHICreateTexture2D(1, 1, { 255, 255, 255, 255 });
	if (!White) return false;
	__WhiteTexture = static_cast<FD3D11Texture*>(White.get())->__View;
	D3D11_SAMPLER_DESC Sampler = {};
	Sampler.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
	Sampler.AddressU = Sampler.AddressV = Sampler.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	Sampler.MaxLOD = D3D11_FLOAT32_MAX;
	if (FAILED(__Device->CreateSamplerState(&Sampler, __TextureSampler.GetAddressOf()))) return false;
	RasterizerDesc.FillMode = D3D11_FILL_SOLID;
	RasterizerDesc.CullMode = D3D11_CULL_NONE;
	RasterizerDesc.DepthClipEnable = TRUE;
	return SUCCEEDED(__Device->CreateRasterizerState(
		&RasterizerDesc,
		__RasterizerState.GetAddressOf()));
}

FViewportRHIRef FD3D11DynamicRHI::RHICreateViewport(
	const FRHIViewportDesc& _Desc)
{
	std::shared_ptr<FD3D11Viewport> Viewport = std::make_shared<FD3D11Viewport>();
	if (false == Viewport->Initialize(*this, _Desc))
		return nullptr;

	return Viewport;
}

bool FD3D11DynamicRHI::RHIResizeViewport(
	FRHIViewport* _Viewport,
	uint32 _SizeX,
	uint32 _SizeY)
{
	FD3D11Viewport* Viewport = dynamic_cast<FD3D11Viewport*>(_Viewport);
	if (nullptr == Viewport || Viewport == __DrawingViewport)
		return false;

	if (__DeviceContext)
		__DeviceContext->OMSetRenderTargets(0, nullptr, nullptr);
	return Viewport->Resize(_SizeX, _SizeY);
}

bool FD3D11DynamicRHI::RHIBeginDrawingViewport(
	FRHIViewport* _Viewport,
	const FColor& _ClearColor)
{
	FD3D11Viewport* Viewport = dynamic_cast<FD3D11Viewport*>(_Viewport);
	if (nullptr == Viewport || nullptr == Viewport->GetRenderTargetView() ||
		nullptr == __DeviceContext || nullptr != __DrawingViewport)
	{
		return false;
	}

	__DrawingViewport = Viewport;
	ID3D11RenderTargetView* RenderTarget = Viewport->GetRenderTargetView();
	__DeviceContext->OMSetRenderTargets(1, &RenderTarget, nullptr);

	D3D11_VIEWPORT D3DViewport = {};
	D3DViewport.Width = static_cast<float>(Viewport->GetSizeX());
	D3DViewport.Height = static_cast<float>(Viewport->GetSizeY());
	D3DViewport.MinDepth = 0.0f;
	D3DViewport.MaxDepth = 1.0f;
	__DeviceContext->RSSetViewports(1, &D3DViewport);

	const float ClearColor[] =
	{
		static_cast<float>(_ClearColor.R) / 255.0f,
		static_cast<float>(_ClearColor.G) / 255.0f,
		static_cast<float>(_ClearColor.B) / 255.0f,
		static_cast<float>(_ClearColor.A) / 255.0f
	};
	__DeviceContext->ClearRenderTargetView(RenderTarget, ClearColor);
	return true;
}

FD3D11DynamicRHI::FD3D11SimpleVertex FD3D11DynamicRHI::MakeVertex(
	float _X,
	float _Y,
	const FColor& _Color) const
{
	const float Width = static_cast<float>(__DrawingViewport->GetSizeX());
	const float Height = static_cast<float>(__DrawingViewport->GetSizeY());
	return
	{
		{ (_X / Width) * 2.0f - 1.0f, 1.0f - (_Y / Height) * 2.0f },
		{
			static_cast<float>(_Color.R) / 255.0f,
			static_cast<float>(_Color.G) / 255.0f,
			static_cast<float>(_Color.B) / 255.0f,
			static_cast<float>(_Color.A) / 255.0f
		}
	};
}

void FD3D11DynamicRHI::DrawVertices(
	const FD3D11SimpleVertex* _Vertices,
	uint32 _VertexCount,
	D3D11_PRIMITIVE_TOPOLOGY _Topology, ID3D11ShaderResourceView* _Texture)
{
	if (nullptr == __DrawingViewport || nullptr == _Vertices ||
		0 == _VertexCount || _VertexCount > 6 || nullptr == __DeviceContext)
	{
		return;
	}

	D3D11_MAPPED_SUBRESOURCE Mapped = {};
	if (FAILED(__DeviceContext->Map(
		__DynamicVertexBuffer.Get(),
		0,
		D3D11_MAP_WRITE_DISCARD,
		0,
		&Mapped)))
	{
		return;
	}

	std::memcpy(Mapped.pData, _Vertices, sizeof(FD3D11SimpleVertex) * _VertexCount);
	__DeviceContext->Unmap(__DynamicVertexBuffer.Get(), 0);

	constexpr UINT Stride = sizeof(FD3D11SimpleVertex);
	constexpr UINT Offset = 0;
	ID3D11Buffer* VertexBuffer = __DynamicVertexBuffer.Get();
	__DeviceContext->IASetInputLayout(__InputLayout.Get());
	__DeviceContext->IASetVertexBuffers(0, 1, &VertexBuffer, &Stride, &Offset);
	__DeviceContext->IASetPrimitiveTopology(_Topology);
	__DeviceContext->VSSetShader(__VertexShader.Get(), nullptr, 0);
	__DeviceContext->PSSetShader(__PixelShader.Get(), nullptr, 0);
	ID3D11ShaderResourceView* Texture = _Texture ? _Texture : __WhiteTexture.Get();
	ID3D11SamplerState* Sampler = __TextureSampler.Get();
	__DeviceContext->PSSetShaderResources(0, 1, &Texture);
	__DeviceContext->PSSetSamplers(0, 1, &Sampler);
	__DeviceContext->RSSetState(__RasterizerState.Get());
	const float BlendFactor[] = { 0.0f, 0.0f, 0.0f, 0.0f };
	__DeviceContext->OMSetBlendState(__BlendState.Get(), BlendFactor, 0xffffffffu);
	__DeviceContext->Draw(_VertexCount, 0);
}

void FD3D11DynamicRHI::RHIDrawLine(
	int32 _StartX,
	int32 _StartY,
	int32 _EndX,
	int32 _EndY,
	const FColor& _Color,
	int32 _Thickness)
{
	if (nullptr == __DrawingViewport)
		return;

	const float HalfThickness = static_cast<float>(_Thickness > 0 ? _Thickness : 1) * 0.5f;
	const float DeltaX = static_cast<float>(_EndX - _StartX);
	const float DeltaY = static_cast<float>(_EndY - _StartY);
	const float Length = std::sqrt(DeltaX * DeltaX + DeltaY * DeltaY);
	if (Length <= 0.0f)
		return;

	const float NormalX = -DeltaY / Length * HalfThickness;
	const float NormalY = DeltaX / Length * HalfThickness;
	const FD3D11SimpleVertex Vertices[] =
	{
		MakeVertex(_StartX + NormalX, _StartY + NormalY, _Color),
		MakeVertex(_EndX + NormalX, _EndY + NormalY, _Color),
		MakeVertex(_EndX - NormalX, _EndY - NormalY, _Color),
		MakeVertex(_StartX + NormalX, _StartY + NormalY, _Color),
		MakeVertex(_EndX - NormalX, _EndY - NormalY, _Color),
		MakeVertex(_StartX - NormalX, _StartY - NormalY, _Color)
	};
	DrawVertices(Vertices, static_cast<uint32>(std::size(Vertices)), D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

void FD3D11DynamicRHI::RHIDrawRectangle(
	int32 _Left,
	int32 _Top,
	int32 _Right,
	int32 _Bottom,
	const FColor& _Color)
{
	if (nullptr == __DrawingViewport)
		return;

	const FD3D11SimpleVertex Vertices[] =
	{
		MakeVertex(static_cast<float>(_Left), static_cast<float>(_Top), _Color),
		MakeVertex(static_cast<float>(_Right), static_cast<float>(_Top), _Color),
		MakeVertex(static_cast<float>(_Right), static_cast<float>(_Bottom), _Color),
		MakeVertex(static_cast<float>(_Left), static_cast<float>(_Top), _Color),
		MakeVertex(static_cast<float>(_Right), static_cast<float>(_Bottom), _Color),
		MakeVertex(static_cast<float>(_Left), static_cast<float>(_Bottom), _Color)
	};
	DrawVertices(Vertices, static_cast<uint32>(std::size(Vertices)), D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

void FD3D11DynamicRHI::RHIEndDrawingViewport(
	FRHIViewport* _Viewport,
	bool _bPresent)
{
	FD3D11Viewport* Viewport = dynamic_cast<FD3D11Viewport*>(_Viewport);
	if (nullptr != Viewport && Viewport == __DrawingViewport)
	{
		__DeviceContext->OMSetRenderTargets(0, nullptr, nullptr);
		if (_bPresent)
			Viewport->Present();
	}

	__DrawingViewport = nullptr;
}

bool FD3D11DynamicRHIModule::IsSupported() const
{
	ComPtr<ID3D11Device> Device;
	ComPtr<ID3D11DeviceContext> DeviceContext;
	D3D_FEATURE_LEVEL FeatureLevel = D3D_FEATURE_LEVEL_9_1;
	return SUCCEEDED(CreateD3D11Device(
		D3D11_CREATE_DEVICE_BGRA_SUPPORT,
		Device.GetAddressOf(),
		&FeatureLevel,
		DeviceContext.GetAddressOf()));
}

std::unique_ptr<FDynamicRHI> FD3D11DynamicRHIModule::CreateRHI()
{
	return std::make_unique<FD3D11DynamicRHI>();
}
