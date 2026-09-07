#pragma once

struct HWND__;
using HWND = HWND__*;
struct ImDrawData;

bool ImGui_ImplGDI_Init(HWND _Window);
void ImGui_ImplGDI_Shutdown();
void ImGui_ImplGDI_NewFrame();
void ImGui_ImplGDI_RenderDrawData(ImDrawData* _DrawData);
