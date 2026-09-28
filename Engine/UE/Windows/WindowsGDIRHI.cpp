#include "pch.h"
#include <cstring>
#pragma comment(lib, "msimg32.lib")
#include "WindowsGDIRHI.h"

namespace
{
	COLORREF ToColorRef(const FColor& _Color)
	{
		return RGB(_Color.R, _Color.G, _Color.B);
	}
}

FWindowsGDIViewport::~FWindowsGDIViewport()
{
	Release();
}

bool FWindowsGDIViewport::Initialize(const FRHIViewportDesc& _Desc)
{
	HWND WindowHandle = static_cast<HWND>(_Desc.WindowHandle);
	if (nullptr == WindowHandle || FALSE == IsWindow(WindowHandle) ||
		0 == _Desc.SizeX || 0 == _Desc.SizeY)
	{
		return false;
	}

	__WindowHandle = WindowHandle;
	return Resize(_Desc.SizeX, _Desc.SizeY);
}

bool FWindowsGDIViewport::Resize(uint32 _SizeX, uint32 _SizeY)
{
	if (nullptr == __WindowHandle || FALSE == IsWindow(__WindowHandle) ||
		0 == _SizeX || 0 == _SizeY)
	{
		return false;
	}

	wil::unique_hdc_window WindowDC = wil::GetDC(__WindowHandle);
	if (!WindowDC)
		return false;

	wil::unique_hdc NewBackBufferDC(CreateCompatibleDC(WindowDC.get()));
	if (!NewBackBufferDC)
		return false;

	wil::unique_hbitmap NewBackBufferBitmap(CreateCompatibleBitmap(
		WindowDC.get(),
		static_cast<int>(_SizeX),
		static_cast<int>(_SizeY)));

	if (!NewBackBufferBitmap)
		return false;

	HGDIOBJ NewPreviousBitmap = SelectObject(
		NewBackBufferDC.get(),
		NewBackBufferBitmap.get());

	if (nullptr == NewPreviousBitmap || HGDI_ERROR == NewPreviousBitmap)
		return false;

	if (__BackBufferDC)
	{
		if (nullptr != __PreviousBitmap)
			SelectObject(__BackBufferDC.get(), __PreviousBitmap);
	}

	__BackBufferBitmap.reset();
	__BackBufferDC.reset();

	__BackBufferDC = std::move(NewBackBufferDC);
	__BackBufferBitmap = std::move(NewBackBufferBitmap);
	__PreviousBitmap = NewPreviousBitmap;
	__SizeX = _SizeX;
	__SizeY = _SizeY;

	return true;
}

void FWindowsGDIViewport::Release()
{
	if (__BackBufferDC && nullptr != __PreviousBitmap)
		SelectObject(__BackBufferDC.get(), __PreviousBitmap);

	__PreviousBitmap = nullptr;
	__BackBufferBitmap.reset();
	__BackBufferDC.reset();
	__SizeX = 0;
	__SizeY = 0;
	__WindowHandle = nullptr;
}

bool FWindowsGDIViewport::Present()
{
	if (nullptr == __WindowHandle || FALSE == IsWindow(__WindowHandle) ||
		!__BackBufferDC || 0 == __SizeX || 0 == __SizeY)
	{
		return false;
	}

	wil::unique_hdc_window WindowDC = wil::GetDC(__WindowHandle);
	if (!WindowDC)
		return false;

	return TRUE == BitBlt(
		WindowDC.get(),
		0,
		0,
		static_cast<int>(__SizeX),
		static_cast<int>(__SizeY),
		__BackBufferDC.get(),
		0,
		0,
		SRCCOPY);
}

FWindowsGDIRHI::~FWindowsGDIRHI()
{
	Shutdown();
}

bool FWindowsGDIRHI::Init()
{
	return true;
}

void FWindowsGDIRHI::Shutdown()
{
	__DrawingViewport = nullptr;
}

FViewportRHIRef FWindowsGDIRHI::RHICreateViewport(
	const FRHIViewportDesc& _Desc)
{
	std::shared_ptr<FWindowsGDIViewport> Viewport =
		std::make_shared<FWindowsGDIViewport>();

	if (false == Viewport->Initialize(_Desc))
		return nullptr;

	return Viewport;
}

bool FWindowsGDIRHI::RHIResizeViewport(
	FRHIViewport* _Viewport,
	uint32 _SizeX,
	uint32 _SizeY)
{
	FWindowsGDIViewport* Viewport =
		dynamic_cast<FWindowsGDIViewport*>(_Viewport);

	if (nullptr == Viewport || Viewport == __DrawingViewport)
		return false;

	return Viewport->Resize(_SizeX, _SizeY);
}

bool FWindowsGDIRHI::RHIBeginDrawingViewport(
	FRHIViewport* _Viewport,
	const FColor& _ClearColor)
{
	__DrawingViewport = dynamic_cast<FWindowsGDIViewport*>(_Viewport);

	if (nullptr == __DrawingViewport)
		return false;

	HDC BackBufferDC = __DrawingViewport->GetBackBufferDC();
	if (nullptr == BackBufferDC)
	{
		__DrawingViewport = nullptr;
		return false;
	}

	const RECT BackBufferRect =
	{
		0,
		0,
		static_cast<LONG>(__DrawingViewport->GetSizeX()),
		static_cast<LONG>(__DrawingViewport->GetSizeY())
	};

	wil::unique_hbrush ClearBrush(
		CreateSolidBrush(ToColorRef(_ClearColor)));

	if (!ClearBrush)
	{
		__DrawingViewport = nullptr;
		return false;
	}

	FillRect(
		BackBufferDC,
		&BackBufferRect,
		ClearBrush.get());

	return true;
}

void FWindowsGDIRHI::RHIDrawLine(int32 _StartX, int32 _StartY, int32 _EndX, int32 _EndY,
	const FColor& _Color, int32 _Thickness)
{
	if (nullptr == __DrawingViewport)
		return;

	HDC BackBufferDC = __DrawingViewport->GetBackBufferDC();
	if (nullptr == BackBufferDC)
		return;

	wil::unique_hpen Pen(CreatePen(PS_SOLID, _Thickness, ToColorRef(_Color)));
	if (!Pen)
		return;

	HGDIOBJ PreviousPen = SelectObject(BackBufferDC, Pen.get());
	MoveToEx(BackBufferDC, _StartX, _StartY, nullptr);
	LineTo(BackBufferDC, _EndX, _EndY);
	SelectObject(BackBufferDC, PreviousPen);
}

namespace
{
    class FGDITexture final : public FRHITexture
    {
    public:
        HDC __DC = nullptr;
        HBITMAP __Bitmap = nullptr;
        HGDIOBJ __Previous = nullptr;
        uint32 __Width = 0, __Height = 0;
        ~FGDITexture() override
        {
            if (__Previous) SelectObject(__DC, __Previous);
            if (__Bitmap) DeleteObject(__Bitmap);
            if (__DC) DeleteDC(__DC);
        }
    };
}

FTextureRHIRef FWindowsGDIRHI::RHICreateTexture2D(uint32 _Width, uint32 _Height, const std::vector<uint8>& _Pixels)
{
    if (!_Width || !_Height || _Width > 8192 || _Height > 8192 || _Pixels.size() != uint64(_Width) * _Height * 4) return nullptr;
    auto Texture = std::make_shared<FGDITexture>();
    Texture->__DC = CreateCompatibleDC(nullptr);
    if (!Texture->__DC) return nullptr;
    BITMAPINFO Info = {};
    Info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    Info.bmiHeader.biWidth = static_cast<LONG>(_Width);
    Info.bmiHeader.biHeight = -static_cast<LONG>(_Height);
    Info.bmiHeader.biPlanes = 1;
    Info.bmiHeader.biBitCount = 32;
    Info.bmiHeader.biCompression = BI_RGB;
    void* Pixels = nullptr;
    Texture->__Bitmap = CreateDIBSection(Texture->__DC, &Info, DIB_RGB_COLORS, &Pixels, nullptr, 0);
    if (!Texture->__Bitmap || !Pixels) return nullptr;
    auto* Target = static_cast<uint8*>(Pixels);
    for (size_t Index = 0; Index < _Pixels.size(); Index += 4)
    {
        const uint32 Alpha = _Pixels[Index + 3];
        Target[Index] = static_cast<uint8>(_Pixels[Index + 2] * Alpha / 255);
        Target[Index + 1] = static_cast<uint8>(_Pixels[Index + 1] * Alpha / 255);
        Target[Index + 2] = static_cast<uint8>(_Pixels[Index] * Alpha / 255);
        Target[Index + 3] = static_cast<uint8>(Alpha);
    }
    Texture->__Previous = SelectObject(Texture->__DC, Texture->__Bitmap);
    Texture->__Width = _Width;
    Texture->__Height = _Height;
    return Texture;
}

void FWindowsGDIRHI::RHIDrawTexture(FRHITexture* _Texture, int32 _X, int32 _Y, int32 _Width, int32 _Height)
{
    const auto* Texture = dynamic_cast<FGDITexture*>(_Texture);
    if (!Texture || !__DrawingViewport || _Width <= 0 || _Height <= 0) return;
    const BLENDFUNCTION Blend = { AC_SRC_OVER, 0, 255, AC_SRC_ALPHA };
    AlphaBlend(__DrawingViewport->GetBackBufferDC(), _X, _Y, _Width, _Height,
        Texture->__DC, 0, 0, Texture->__Width, Texture->__Height, Blend);
}

void FWindowsGDIRHI::RHIDrawQuad(const FVector* _Corners, const FColor& _Color, FRHITexture* _Texture)
{
    if (!__DrawingViewport || !_Corners) return;
    const auto DC = __DrawingViewport->GetBackBufferDC();
    const auto* Texture = dynamic_cast<FGDITexture*>(_Texture);
    const int Saved = SaveDC(DC);
    if (!Saved) return;
    if (Texture)
    {
        SetGraphicsMode(DC, GM_ADVANCED);
        const XFORM Transform = {
            float((_Corners[1].X - _Corners[0].X) / Texture->__Width),
            float((_Corners[1].Y - _Corners[0].Y) / Texture->__Width),
            float((_Corners[3].X - _Corners[0].X) / Texture->__Height),
            float((_Corners[3].Y - _Corners[0].Y) / Texture->__Height),
            float(_Corners[0].X), float(_Corners[0].Y)};
        if (SetWorldTransform(DC, &Transform))
        {
            const BLENDFUNCTION Blend = {AC_SRC_OVER, 0, 255, AC_SRC_ALPHA};
            AlphaBlend(DC, 0, 0, Texture->__Width, Texture->__Height,
                Texture->__DC, 0, 0, Texture->__Width, Texture->__Height, Blend);
        }
    }
    else
    {
        POINT Points[4];
        for (int Index = 0; Index < 4; ++Index) Points[Index] = {LONG(_Corners[Index].X), LONG(_Corners[Index].Y)};
        wil::unique_hbrush Brush(CreateSolidBrush(ToColorRef(_Color)));
        SelectObject(DC, Brush.get()); SelectObject(DC, GetStockObject(NULL_PEN));
        Polygon(DC, Points, 4);
        RestoreDC(DC, Saved);
        return;
    }
    RestoreDC(DC, Saved);
}

void FWindowsGDIRHI::RHIDrawRectangle(int32 _Left, int32 _Top, int32 _Right, int32 _Bottom,
	const FColor& _Color)
{
	if (nullptr == __DrawingViewport)
		return;

	HDC BackBufferDC = __DrawingViewport->GetBackBufferDC();
	if (nullptr == BackBufferDC)
		return;

	wil::unique_hbrush Brush(CreateSolidBrush(ToColorRef(_Color)));
	if (!Brush)
		return;

	RECT RectangleArea = { _Left, _Top, _Right, _Bottom };
	FillRect(BackBufferDC, &RectangleArea, Brush.get());
}

void FWindowsGDIRHI::RHIEndDrawingViewport(FRHIViewport* _Viewport, bool _bPresent)
{
	FWindowsGDIViewport* Viewport =
		dynamic_cast<FWindowsGDIViewport*>(_Viewport);

	if (nullptr != Viewport && Viewport == __DrawingViewport && _bPresent)
		Viewport->Present();

	__DrawingViewport = nullptr;
}

bool FWindowsGDIRHIModule::IsSupported() const
{
	return true;
}

std::unique_ptr<FDynamicRHI> FWindowsGDIRHIModule::CreateRHI()
{
	return std::make_unique<FWindowsGDIRHI>();
}
