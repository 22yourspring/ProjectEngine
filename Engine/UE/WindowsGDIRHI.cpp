#include "pch.h"
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
