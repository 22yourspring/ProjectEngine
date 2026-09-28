#include "framework.h"
#include "EditorImGui.h"
#include "UE/Engine.h"
#include "UE/ResourceEngineSystem.h"
#include "UE/Material.h"
#include "UE/MaterialInstanceConstant.h"
#include "UE/Texture2D.h"
#include "UE/Package.h"
#include <algorithm>

void FEditorImGui::DrawMaterialCreation(const std::filesystem::path& _Directory)
{
    if (ImGui::Button("+ Add")) ImGui::OpenPopup("AddContentAsset");
    if (ImGui::BeginPopup("AddContentAsset"))
    {
        if (ImGui::MenuItem("Material"))
        {
            __CreateMaterialParent = nullptr;
            __MaterialDirectory = _Directory;
            strcpy_s(__MaterialName, "M_NewMaterial");
            __ShowMaterialCreate = true;
        }
        ImGui::EndPopup();
    }
    if (__ShowMaterialCreate) { ImGui::OpenPopup("Create Material Asset"); __ShowMaterialCreate = false; }
    if (ImGui::BeginPopupModal("Create Material Asset", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::TextUnformatted(__CreateMaterialParent ? "Material Instance" : "Material");
        ImGui::InputText("Asset Name", __MaterialName, sizeof(__MaterialName));
        if (ImGui::Button("Create"))
        {
            auto* Resources = GEngine ? GEngine->GetEngineSystem<ResourceEngineSystem>() : nullptr;
            if (Resources)
            {
                auto& Store = Resources->GetPackageStore();
                FString Path, Error;
                const auto File = __MaterialDirectory / (FString(__MaterialName).ToWide() + L".uasset");
                std::unique_ptr<UMaterialInterface> Asset;
                if (auto* Parent = dynamic_cast<UMaterialInterface*>(__CreateMaterialParent))
                {
                    auto Instance = std::make_unique<UMaterialInstanceConstant>();
                    Instance->SetParentEditorOnly(Parent);
                    Asset = std::move(Instance);
                }
                else Asset = std::make_unique<UMaterial>();
                if (Store.GetObjectPath(File, Path) && Store.Save(Path, *Asset, FString(), Error))
                {
                    __NextAssetRefresh = 0;
                    OpenAsset(File);
                    ImGui::CloseCurrentPopup();
                }
                else __AssetMessage = Error.IsEmpty() ? "Use a valid asset name inside a Content folder." : Error.ToUtf8();
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel")) ImGui::CloseCurrentPopup();
        if (!__AssetMessage.IsEmpty()) ImGui::TextWrapped("%s", __AssetMessage.ToUtf8().c_str());
        ImGui::EndPopup();
    }
}

void FEditorImGui::DrawMaterialEditor()
{
    auto* Material = dynamic_cast<UMaterialInterface*>(__PreviewAsset);
    if (!Material) return;
    auto* Base = dynamic_cast<UMaterial*>(Material);
    auto* Instance = dynamic_cast<UMaterialInstanceConstant*>(Material);
    auto* Package = dynamic_cast<UPackage*>(Material->GetOuter());
    const auto Title = Material->GetName().ToUtf8() + (Package && Package->IsDirty() ? "*" : "") +
        (Instance ? " - Material Instance Editor###AssetEditor" : " - Material Editor###AssetEditor");
    ImGui::SetNextWindowSize(ImVec2(880, 620), ImGuiCond_FirstUseEver);
    if (ImGui::Begin(Title.c_str(), &__ShowAsset))
    {
        if (ImGui::Button("Save"))
        {
            auto* Resources = GEngine ? GEngine->GetEngineSystem<ResourceEngineSystem>() : nullptr;
            FString Error;
            if (Resources && Resources->GetPackageStore().Save(Material->GetPathName(), *Material, FString(), Error, true))
                __AssetMessage = "Material saved.";
            else __AssetMessage = Error;
        }
        ImGui::Separator();
        ImGui::TextUnformatted("Surface / Unlit");
        ImGui::BeginChild("MaterialDetails", ImVec2(360, 0), ImGuiChildFlags_Borders);
        if (Instance)
        {
            UObject* Parent = Instance->GetParent();
            if (DrawAssetProperty("Parent", Parent, 4))
                if (!Instance->SetParentEditorOnly(dynamic_cast<UMaterialInterface*>(Parent)))
                    __AssetMessage = "A parent is required; cyclic parent references are not allowed.";
            if (ImGui::Button("Reset All Overrides")) Instance->ClearParameterValues();
        }
        UTexture* Texture = nullptr;
        Material->GetTextureParameterValue(FName(TEXT("BaseColorTexture")), Texture);
        UObject* TextureAsset = Texture;
        if (DrawAssetProperty("BaseColorTexture", TextureAsset, 1))
        {
            if (Base) Base->SetTextureParameterValueEditorOnly(FName(TEXT("BaseColorTexture")), static_cast<UTexture*>(TextureAsset));
            if (Instance) Instance->SetTextureParameterValueEditorOnly(FName(TEXT("BaseColorTexture")), static_cast<UTexture*>(TextureAsset));
        }
        FLinearColor Color(1, 1, 1, 1);
        float Opacity = 1;
        Material->GetVectorParameterValue(FName(TEXT("BaseColor")), Color);
        Material->GetScalarParameterValue(FName(TEXT("Opacity")), Opacity);
        float Values[] = {Color.R, Color.G, Color.B, Color.A};
        if (ImGui::ColorEdit4("BaseColor", Values))
        {
            const FLinearColor Value(Values[0], Values[1], Values[2], Values[3]);
            if (Base) Base->SetVectorParameterValueEditorOnly(FName(TEXT("BaseColor")), Value);
            if (Instance) Instance->SetVectorParameterValueEditorOnly(FName(TEXT("BaseColor")), Value);
        }
        if (ImGui::SliderFloat("Opacity", &Opacity, 0, 1))
        {
            if (Base) Base->SetScalarParameterValueEditorOnly(FName(TEXT("Opacity")), Opacity);
            if (Instance) Instance->SetScalarParameterValueEditorOnly(FName(TEXT("Opacity")), Opacity);
        }
        if (Instance)
            ImGui::Text("Overrides: Texture %s / Color %s / Opacity %s", Instance->HasTextureOverride() ? "Yes" : "No",
                Instance->HasVectorOverride() ? "Yes" : "No", Instance->HasScalarOverride() ? "Yes" : "No");
        ImGui::TextWrapped("Edits update meshes using this material. Save stores the asset; save the level to store material slot assignments.");
        if (!__AssetMessage.IsEmpty()) ImGui::TextWrapped("%s", __AssetMessage.ToUtf8().c_str());
        ImGui::EndChild();
        ImGui::SameLine();
        ImGui::BeginChild("MaterialPreview", ImVec2(0, 0), ImGuiChildFlags_Borders);
        const auto Proxy = Material->GetRenderProxy();
        std::shared_ptr<const FMaterialRenderData> Data;
        { std::lock_guard<std::mutex> Lock(Proxy->__Mutex); Data = Proxy->__Data; }
        if (__MaterialPreviewData != Data)
        {
            D3D11_TEXTURE2D_DESC Desc = {};
            Desc.Width = Data->__Width; Desc.Height = Data->__Height;
            Desc.MipLevels = Desc.ArraySize = 1; Desc.SampleDesc.Count = 1;
            Desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; Desc.Usage = D3D11_USAGE_IMMUTABLE;
            Desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
            D3D11_SUBRESOURCE_DATA Pixels = {Data->__Pixels.data(), Data->__Width * 4, 0};
            Microsoft::WRL::ComPtr<ID3D11Texture2D> Resource;
            Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> View;
            if (SUCCEEDED(__Device->CreateTexture2D(&Desc, &Pixels, &Resource)) &&
                SUCCEEDED(__Device->CreateShaderResourceView(Resource.Get(), nullptr, &View)))
            {
                if (__PreviewTexture) __RetiredTextures.push_back(__PreviewTexture);
                __PreviewTexture = std::move(View);
                __MaterialPreviewData = Data;
            }
        }
        if (__PreviewTexture)
        {
            const auto Available = ImGui::GetContentRegionAvail();
            const float Side = (std::min)(Available.x, Available.y);
            if (Side > 0) ImGui::Image(reinterpret_cast<ImTextureID>(__PreviewTexture.Get()), ImVec2(Side, Side));
        }
        ImGui::EndChild();
    }
    ImGui::End();
}
