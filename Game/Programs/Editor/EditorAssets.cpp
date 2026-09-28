#include "framework.h"
#include "EditorImGui.h"
#include "AssetImport.h"
#include "UE/ResourceEngineSystem.h"
#include "UE/Texture2D.h"
#include "UE/SoundWave.h"
#include "UE/AudioComponent.h"
#include "UE/GameEngine.h"
#include "UE/PathEngineSystem.h"
#include "EditorActorSubsystem.h"
#include "UE/WorldPersistence.h"
#include <fstream>
#include <cmath>
#include <commdlg.h>
#include <algorithm>

bool FEditorImGui::IsEditorMap(const std::filesystem::path& _Path) const
{
    if (_wcsicmp(_Path.extension().c_str(), L".umap")) return false;
    std::vector<uint8> Data; int Mode = -1; FString Error;
    if (FWorldPersistence::Read(_Path, Data, Mode, Error)) return true;
    std::ifstream Input(_Path);
    std::string Tag, Name;
    if (!(Input >> Tag >> Name) || Tag != "Level" || Name != FString(_Path.stem().wstring()).ToUtf8()) return false;
    while (Input >> Tag)
    {
        double X, Y, Z;
        if (Tag != "PlayerLocation" || !(Input >> X >> Y >> Z) ||
            !std::isfinite(X) || !std::isfinite(Y) || !std::isfinite(Z)) return false;
    }
    return Input.eof();
}

std::filesystem::path FEditorImGui::GetEditorActorsSavePath() const
{
    auto* Paths = GEngine ? GEngine->GetEngineSystem<PathEngineSystem>() : nullptr;
    if (!Paths) return {};
    if (__EditedMap.empty()) return Paths->GetProjectSavedDirectory() / L"Config/EditorAssetActors.txt";
    auto Relative = __EditedMap.lexically_relative(Paths->GetProjectContentDirectory());
    if (!Relative.empty() && *Relative.begin() == L"..")
        Relative = std::filesystem::path(L"Engine") / __EditedMap.lexically_relative(Paths->GetEngineContentDirectory());
    Relative.replace_extension(L".actors");
    return Paths->GetProjectSavedDirectory() / L"Config/EditorLevels" / Relative;
}

void FEditorImGui::OpenEditorMap(const std::filesystem::path& _Path)
{
    RequestFileAction(EFileAction::OpenLevel, _Path);
}

void FEditorImGui::OpenAsset(const std::filesystem::path& _Path)
{
    if (_wcsicmp(_Path.extension().c_str(), L".uasset")) return;
    __PreviewPath = _Path;
    __PreviewAudio.reset();
    if (__PreviewTexture) __RetiredTextures.push_back(__PreviewTexture);
    __PreviewTexture.Reset();
    __MaterialPreviewData.reset();
    __PreviewAsset = nullptr;
    __AssetMessage.Empty();
    __ShowAsset = true;
    auto* Resources = GEngine ? GEngine->GetEngineSystem<ResourceEngineSystem>() : nullptr;
    FString ObjectPath, Error;
    if (!Resources || !Resources->GetPackageStore().GetObjectPath(_Path, ObjectPath))
    {
        __AssetMessage = "Select an asset inside a Content folder.";
        return;
    }
    __PreviewAsset = Resources->GetPackageStore().Load(ObjectPath, Error);
    if (!__PreviewAsset) { __AssetMessage = Error; return; }
    if (const auto* Texture = dynamic_cast<UTexture2D*>(__PreviewAsset))
    {
        D3D11_TEXTURE2D_DESC Desc = {};
        Desc.Width = Texture->GetSizeX();
        Desc.Height = Texture->GetSizeY();
        Desc.MipLevels = Desc.ArraySize = 1;
        Desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        Desc.SampleDesc.Count = 1;
        Desc.Usage = D3D11_USAGE_IMMUTABLE;
        Desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        D3D11_SUBRESOURCE_DATA Data = { Texture->GetPixels().data(), Desc.Width * 4, 0 };
        Microsoft::WRL::ComPtr<ID3D11Texture2D> Resource;
        if (FAILED(__Device->CreateTexture2D(&Desc, &Data, Resource.GetAddressOf())) ||
            FAILED(__Device->CreateShaderResourceView(Resource.Get(), nullptr, __PreviewTexture.GetAddressOf())))
            __AssetMessage = "Could not create the texture preview.";
    }
    else if (auto* Sound = dynamic_cast<USoundWave*>(__PreviewAsset))
    {
        __PreviewWaveform.assign(256, 0.0f);
        const auto& PCM = Sound->GetPCMData();
        const size_t Samples = PCM.size() / 2;
        for (size_t Bin = 0; Bin < __PreviewWaveform.size(); ++Bin)
        {
            const size_t Begin = Samples * Bin / __PreviewWaveform.size();
            const size_t End = Samples * (Bin + 1) / __PreviewWaveform.size();
            const size_t Step = (std::max)(size_t(1), (End - Begin) / 1024);
            for (size_t I = Begin; I < End; I += Step)
            {
                const int16 Sample = static_cast<int16>(PCM[I*2] | (uint16(PCM[I*2+1]) << 8));
                __PreviewWaveform[Bin] = (std::max)(__PreviewWaveform[Bin], std::abs(float(Sample)) / 32768.0f);
            }
        }
        __PreviewAudio = std::make_unique<UAudioComponent>();
        __PreviewAudio->SetSound(Sound);
    }
}
