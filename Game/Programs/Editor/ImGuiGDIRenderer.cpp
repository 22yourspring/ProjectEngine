#include "ImGuiGDIRenderer.h"

#include "ThirdParty/ImGui/imgui.h"

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

namespace
{
    struct FGDIRendererData
    {
        HWND __Window = nullptr;
        BITMAPINFO __BitmapInfo = {};
        std::vector<std::uint32_t> __BackBuffer;
        std::vector<unsigned char> __FontPixels;
        int __Width = 0;
        int __Height = 0;
        int __FontWidth = 0;
        int __FontHeight = 0;
    };

    FGDIRendererData* GRenderer = nullptr;

    float Edge(const ImVec2& _A, const ImVec2& _B, float _X, float _Y)
    {
        return (_X - _A.x) * (_B.y - _A.y) - (_Y - _A.y) * (_B.x - _A.x);
    }

    unsigned char ColorChannel(ImU32 _Color, int _Shift)
    {
        return static_cast<unsigned char>((_Color >> _Shift) & 0xffu);
    }

    void DrawTriangle(
        const ImDrawVert& _V0,
        const ImDrawVert& _V1,
        const ImDrawVert& _V2,
        const ImVec4& _ClipRect,
        const ImVec2& _DisplayPos)
    {
        FGDIRendererData& Renderer = *GRenderer;
        const ImVec2 P0(_V0.pos.x - _DisplayPos.x, _V0.pos.y - _DisplayPos.y);
        const ImVec2 P1(_V1.pos.x - _DisplayPos.x, _V1.pos.y - _DisplayPos.y);
        const ImVec2 P2(_V2.pos.x - _DisplayPos.x, _V2.pos.y - _DisplayPos.y);
        const float Area = Edge(P0, P1, P2.x, P2.y);
        if (std::abs(Area) < 0.0001f)
            return;

        const int ClipLeft = std::clamp(static_cast<int>(_ClipRect.x - _DisplayPos.x), 0, Renderer.__Width);
        const int ClipTop = std::clamp(static_cast<int>(_ClipRect.y - _DisplayPos.y), 0, Renderer.__Height);
        const int ClipRight = std::clamp(static_cast<int>(std::ceil(_ClipRect.z - _DisplayPos.x)), 0, Renderer.__Width);
        const int ClipBottom = std::clamp(static_cast<int>(std::ceil(_ClipRect.w - _DisplayPos.y)), 0, Renderer.__Height);

        const int MinX = std::max(ClipLeft, static_cast<int>(std::floor(std::min({ P0.x, P1.x, P2.x }))));
        const int MinY = std::max(ClipTop, static_cast<int>(std::floor(std::min({ P0.y, P1.y, P2.y }))));
        const int MaxX = std::min(ClipRight, static_cast<int>(std::ceil(std::max({ P0.x, P1.x, P2.x }))));
        const int MaxY = std::min(ClipBottom, static_cast<int>(std::ceil(std::max({ P0.y, P1.y, P2.y }))));
        if (MinX >= MaxX || MinY >= MaxY)
            return;

        for (int Y = MinY; Y < MaxY; ++Y)
        {
            for (int X = MinX; X < MaxX; ++X)
            {
                const float SampleX = static_cast<float>(X) + 0.5f;
                const float SampleY = static_cast<float>(Y) + 0.5f;
                const float W0 = Edge(P1, P2, SampleX, SampleY) / Area;
                const float W1 = Edge(P2, P0, SampleX, SampleY) / Area;
                const float W2 = 1.0f - W0 - W1;
                if (W0 < 0.0f || W1 < 0.0f || W2 < 0.0f)
                    continue;

                const float U = _V0.uv.x * W0 + _V1.uv.x * W1 + _V2.uv.x * W2;
                const float V = _V0.uv.y * W0 + _V1.uv.y * W1 + _V2.uv.y * W2;
                const int TextureX = std::clamp(static_cast<int>(U * Renderer.__FontWidth), 0, Renderer.__FontWidth - 1);
                const int TextureY = std::clamp(static_cast<int>(V * Renderer.__FontHeight), 0, Renderer.__FontHeight - 1);
                const unsigned char* Texel = &Renderer.__FontPixels[
                    static_cast<size_t>((TextureY * Renderer.__FontWidth + TextureX) * 4)];

                const float VertexR = ColorChannel(_V0.col, IM_COL32_R_SHIFT) * W0 + ColorChannel(_V1.col, IM_COL32_R_SHIFT) * W1 + ColorChannel(_V2.col, IM_COL32_R_SHIFT) * W2;
                const float VertexG = ColorChannel(_V0.col, IM_COL32_G_SHIFT) * W0 + ColorChannel(_V1.col, IM_COL32_G_SHIFT) * W1 + ColorChannel(_V2.col, IM_COL32_G_SHIFT) * W2;
                const float VertexB = ColorChannel(_V0.col, IM_COL32_B_SHIFT) * W0 + ColorChannel(_V1.col, IM_COL32_B_SHIFT) * W1 + ColorChannel(_V2.col, IM_COL32_B_SHIFT) * W2;
                const float VertexA = ColorChannel(_V0.col, IM_COL32_A_SHIFT) * W0 + ColorChannel(_V1.col, IM_COL32_A_SHIFT) * W1 + ColorChannel(_V2.col, IM_COL32_A_SHIFT) * W2;
                const float Alpha = (VertexA * static_cast<float>(Texel[3])) / 65025.0f;
                if (Alpha <= 0.0f)
                    continue;

                const int SourceR = static_cast<int>(VertexR * static_cast<float>(Texel[0]) / 255.0f);
                const int SourceG = static_cast<int>(VertexG * static_cast<float>(Texel[1]) / 255.0f);
                const int SourceB = static_cast<int>(VertexB * static_cast<float>(Texel[2]) / 255.0f);
                std::uint32_t& Destination = Renderer.__BackBuffer[static_cast<size_t>(Y * Renderer.__Width + X)];
                const int DestinationB = static_cast<int>(Destination & 0xffu);
                const int DestinationG = static_cast<int>((Destination >> 8) & 0xffu);
                const int DestinationR = static_cast<int>((Destination >> 16) & 0xffu);
                const int ResultR = static_cast<int>(SourceR * Alpha + DestinationR * (1.0f - Alpha));
                const int ResultG = static_cast<int>(SourceG * Alpha + DestinationG * (1.0f - Alpha));
                const int ResultB = static_cast<int>(SourceB * Alpha + DestinationB * (1.0f - Alpha));
                Destination = static_cast<std::uint32_t>(ResultB | (ResultG << 8) | (ResultR << 16));
            }
        }
    }
}

bool ImGui_ImplGDI_Init(HWND _Window)
{
    if (nullptr == _Window || nullptr != GRenderer)
        return false;

    GRenderer = new FGDIRendererData;
    GRenderer->__Window = _Window;
    ImGuiIO& IO = ImGui::GetIO();
    IO.BackendRendererName = "imgui_impl_gdi_educational";

    unsigned char* FontPixels = nullptr;
    IO.Fonts->GetTexDataAsRGBA32(
        &FontPixels,
        &GRenderer->__FontWidth,
        &GRenderer->__FontHeight);
    const size_t FontByteCount = static_cast<size_t>(
        GRenderer->__FontWidth * GRenderer->__FontHeight * 4);
    GRenderer->__FontPixels.assign(FontPixels, FontPixels + FontByteCount);
    IO.Fonts->SetTexID(static_cast<ImTextureID>(1));
    return true;
}

void ImGui_ImplGDI_Shutdown()
{
    if (nullptr == GRenderer)
        return;

    ImGui::GetIO().BackendRendererName = nullptr;
    ImGui::GetIO().Fonts->SetTexID(static_cast<ImTextureID>(0));
    delete GRenderer;
    GRenderer = nullptr;
}

void ImGui_ImplGDI_NewFrame()
{
    if (nullptr == GRenderer)
        return;

    RECT ClientRect = {};
    GetClientRect(GRenderer->__Window, &ClientRect);
    const int Width = ClientRect.right - ClientRect.left;
    const int Height = ClientRect.bottom - ClientRect.top;
    if (Width <= 0 || Height <= 0 ||
        (Width == GRenderer->__Width && Height == GRenderer->__Height))
    {
        return;
    }

    GRenderer->__Width = Width;
    GRenderer->__Height = Height;
    GRenderer->__BackBuffer.resize(static_cast<size_t>(Width * Height));
    GRenderer->__BitmapInfo = {};
    GRenderer->__BitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    GRenderer->__BitmapInfo.bmiHeader.biWidth = Width;
    GRenderer->__BitmapInfo.bmiHeader.biHeight = -Height;
    GRenderer->__BitmapInfo.bmiHeader.biPlanes = 1;
    GRenderer->__BitmapInfo.bmiHeader.biBitCount = 32;
    GRenderer->__BitmapInfo.bmiHeader.biCompression = BI_RGB;
}

void ImGui_ImplGDI_RenderDrawData(ImDrawData* _DrawData)
{
    if (nullptr == GRenderer || nullptr == _DrawData ||
        GRenderer->__Width <= 0 || GRenderer->__Height <= 0)
    {
        return;
    }

    std::fill(GRenderer->__BackBuffer.begin(), GRenderer->__BackBuffer.end(), 0x00111113u);
    for (int ListIndex = 0; ListIndex < _DrawData->CmdListsCount; ++ListIndex)
    {
        const ImDrawList* DrawList = _DrawData->CmdLists[ListIndex];
        for (const ImDrawCmd& Command : DrawList->CmdBuffer)
        {
            if (nullptr != Command.UserCallback)
            {
                if (Command.UserCallback != ImDrawCallback_ResetRenderState)
                    Command.UserCallback(DrawList, &Command);
                continue;
            }

            const ImDrawIdx* Indices = DrawList->IdxBuffer.Data + Command.IdxOffset;
            for (unsigned int Index = 0; Index + 2 < Command.ElemCount; Index += 3)
            {
                DrawTriangle(
                    DrawList->VtxBuffer[Command.VtxOffset + Indices[Index]],
                    DrawList->VtxBuffer[Command.VtxOffset + Indices[Index + 1]],
                    DrawList->VtxBuffer[Command.VtxOffset + Indices[Index + 2]],
                    Command.ClipRect,
                    _DrawData->DisplayPos);
            }
        }
    }

    HDC WindowDC = GetDC(GRenderer->__Window);
    if (nullptr != WindowDC)
    {
        StretchDIBits(
            WindowDC,
            0, 0, GRenderer->__Width, GRenderer->__Height,
            0, 0, GRenderer->__Width, GRenderer->__Height,
            GRenderer->__BackBuffer.data(),
            &GRenderer->__BitmapInfo,
            DIB_RGB_COLORS,
            SRCCOPY);
        ReleaseDC(GRenderer->__Window, WindowDC);
    }
}
