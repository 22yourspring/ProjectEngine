#include "pch.h"
#include "Material.h"
#include "MaterialInstanceConstant.h"
#include "MaterialInstanceDynamic.h"
#include "Texture2D.h"
#include "Archive.h"
#include "Package.h"
#include <algorithm>
#include <cmath>

namespace
{
    std::recursive_mutex& MaterialMutex()
    {
        static std::recursive_mutex Value;
        return Value;
    }
    std::vector<UMaterialInterface*>& Materials()
    {
        static std::vector<UMaterialInterface*> Values;
        return Values;
    }
    bool Named(FName _Name, const wchar_t* _Text)
    {
        return _wcsicmp(_Name.ToString().ToWide().c_str(), _Text) == 0;
    }
    bool ValidColor(FLinearColor _Value)
    {
        return std::isfinite(_Value.R) && std::isfinite(_Value.G) && std::isfinite(_Value.B) && std::isfinite(_Value.A);
    }
    void TextureReference(FArchive& _Archive, UTexture*& _Texture)
    {
        UObject* Object = _Texture;
        _Archive.AssetReference(Object);
        if (_Archive.IsLoading())
        {
            _Texture = dynamic_cast<UTexture2D*>(Object);
            if (Object && !_Texture) _Archive.SetError();
        }
    }
    void Values(FArchive& _Archive, UTexture*& _Texture, FLinearColor& _Color, float& _Opacity)
    {
        TextureReference(_Archive, _Texture);
        _Archive.Serialize(&_Color.R, sizeof(float));
        _Archive.Serialize(&_Color.G, sizeof(float));
        _Archive.Serialize(&_Color.B, sizeof(float));
        _Archive.Serialize(&_Color.A, sizeof(float));
        _Archive.Serialize(&_Opacity, sizeof(float));
        if (!ValidColor(_Color) || !std::isfinite(_Opacity) || _Opacity < 0 || _Opacity > 1) _Archive.SetError();
    }
}

UMaterialInterface::UMaterialInterface() : __RenderProxy(std::make_shared<FMaterialRenderProxy>())
{
}

UMaterialInterface::~UMaterialInterface()
{
    UnregisterMaterial();
}

UMaterial::~UMaterial() { UnregisterMaterial(); }
UMaterialInstance::~UMaterialInstance() { UnregisterMaterial(); }

void UMaterialInterface::UnregisterMaterial()
{
    std::lock_guard<std::recursive_mutex> MaterialLock(MaterialMutex());
    auto& List = Materials();
    List.erase(std::remove(List.begin(), List.end(), this), List.end());
}

void UMaterialInterface::RefreshAllMaterials()
{
    std::lock_guard<std::recursive_mutex> MaterialLock(MaterialMutex());
    for (auto* Material : Materials())
    {
        UTexture* Texture = nullptr;
        FLinearColor Color(1, 1, 1, 1);
        float Opacity = 1;
        Material->GetTextureParameterValue(FName(TEXT("BaseColorTexture")), Texture);
        Material->GetVectorParameterValue(FName(TEXT("BaseColor")), Color);
        Material->GetScalarParameterValue(FName(TEXT("Opacity")), Opacity);
        auto Data = std::make_shared<FMaterialRenderData>();
        if (auto* Image = dynamic_cast<UTexture2D*>(Texture); Image && !Image->GetPixels().empty())
        {
            Data->__Width = Image->GetSizeX(); Data->__Height = Image->GetSizeY();
            Data->__Pixels = Image->GetPixels();
        }
        const float Channels[] = {Color.R, Color.G, Color.B, Color.A * Opacity};
        for (size_t I = 0; I < Data->__Pixels.size(); ++I)
            Data->__Pixels[I] = static_cast<uint8>(std::clamp(Data->__Pixels[I] * Channels[I % 4], 0.0f, 255.0f) + 0.5f);
        const auto Proxy = Material->GetRenderProxy();
        std::lock_guard<std::mutex> Lock(Proxy->__Mutex);
        Proxy->__Data = std::move(Data);
    }
}

void UMaterialInterface::MaterialChanged()
{
    std::lock_guard<std::recursive_mutex> MaterialLock(MaterialMutex());
    auto& List = Materials();
    if (std::find(List.begin(), List.end(), this) == List.end()) List.push_back(this);
    if (auto* Package = dynamic_cast<UPackage*>(GetOuter())) Package->SetDirtyFlag(true);
    RefreshAllMaterials();
}

void UMaterialInterface::PostLoad()
{
    std::lock_guard<std::recursive_mutex> MaterialLock(MaterialMutex());
    auto& List = Materials();
    if (std::find(List.begin(), List.end(), this) == List.end()) List.push_back(this);
    RefreshAllMaterials();
}

bool UMaterial::GetTextureParameterValue(FName _Name, UTexture*& _Value) const
{
    std::lock_guard<std::recursive_mutex> MaterialLock(MaterialMutex());
    if (!Named(_Name, L"BaseColorTexture")) return false;
    _Value = __Texture; return true;
}
bool UMaterial::GetVectorParameterValue(FName _Name, FLinearColor& _Value) const
{
    std::lock_guard<std::recursive_mutex> MaterialLock(MaterialMutex());
    if (!Named(_Name, L"BaseColor")) return false;
    _Value = __BaseColor; return true;
}
bool UMaterial::GetScalarParameterValue(FName _Name, float& _Value) const
{
    std::lock_guard<std::recursive_mutex> MaterialLock(MaterialMutex());
    if (!Named(_Name, L"Opacity")) return false;
    _Value = __Opacity; return true;
}
bool UMaterial::SetTextureParameterValueEditorOnly(FName _Name, UTexture* _Value)
{
    std::lock_guard<std::recursive_mutex> MaterialLock(MaterialMutex());
    if (!Named(_Name, L"BaseColorTexture") || (_Value && !dynamic_cast<UTexture2D*>(_Value))) return false;
    __Texture = _Value; MaterialChanged(); return true;
}
bool UMaterial::SetVectorParameterValueEditorOnly(FName _Name, FLinearColor _Value)
{
    std::lock_guard<std::recursive_mutex> MaterialLock(MaterialMutex());
    if (!Named(_Name, L"BaseColor") || !ValidColor(_Value)) return false;
    __BaseColor = _Value; MaterialChanged(); return true;
}
bool UMaterial::SetScalarParameterValueEditorOnly(FName _Name, float _Value)
{
    std::lock_guard<std::recursive_mutex> MaterialLock(MaterialMutex());
    if (!Named(_Name, L"Opacity") || !std::isfinite(_Value)) return false;
    __Opacity = std::clamp(_Value, 0.0f, 1.0f); MaterialChanged(); return true;
}
void UMaterial::Serialize(FArchive& _Archive) { Values(_Archive, __Texture, __BaseColor, __Opacity); }

UMaterial* UMaterialInstance::GetMaterial() { return __Parent ? __Parent->GetMaterial() : nullptr; }
bool UMaterialInstance::SetParentInternal(UMaterialInterface* _Parent)
{
    std::lock_guard<std::recursive_mutex> MaterialLock(MaterialMutex());
    if (!_Parent) return false;
    auto* Current = _Parent;
    for (size_t Depth = 0; Current; ++Depth)
    {
        if (Current == this || Depth >= 64) return false;
        auto* Instance = dynamic_cast<UMaterialInstance*>(Current);
        Current = Instance ? Instance->GetParent() : nullptr;
    }
    __Parent = _Parent; MaterialChanged(); return true;
}
bool UMaterialInstance::GetTextureParameterValue(FName _Name, UTexture*& _Value) const
{
    std::lock_guard<std::recursive_mutex> MaterialLock(MaterialMutex());
    if (__TextureOverride && Named(_Name, L"BaseColorTexture")) { _Value = __Texture; return true; }
    return __Parent && __Parent->GetTextureParameterValue(_Name, _Value);
}
bool UMaterialInstance::GetVectorParameterValue(FName _Name, FLinearColor& _Value) const
{
    std::lock_guard<std::recursive_mutex> MaterialLock(MaterialMutex());
    if (__ColorOverride && Named(_Name, L"BaseColor")) { _Value = __BaseColor; return true; }
    return __Parent && __Parent->GetVectorParameterValue(_Name, _Value);
}
bool UMaterialInstance::GetScalarParameterValue(FName _Name, float& _Value) const
{
    std::lock_guard<std::recursive_mutex> MaterialLock(MaterialMutex());
    if (__OpacityOverride && Named(_Name, L"Opacity")) { _Value = __Opacity; return true; }
    return __Parent && __Parent->GetScalarParameterValue(_Name, _Value);
}
void UMaterialInstance::SetTextureParameterValueInternal(FName _Name, UTexture* _Value)
{
    std::lock_guard<std::recursive_mutex> MaterialLock(MaterialMutex());
    UTexture* Existing = nullptr;
    if (!__Parent || !__Parent->GetTextureParameterValue(_Name, Existing) || (_Value && !dynamic_cast<UTexture2D*>(_Value))) return;
    __Texture = _Value; __TextureOverride = true; MaterialChanged();
}
void UMaterialInstance::SetVectorParameterValueInternal(FName _Name, FLinearColor _Value)
{
    std::lock_guard<std::recursive_mutex> MaterialLock(MaterialMutex());
    FLinearColor Existing;
    if (!ValidColor(_Value) || !__Parent || !__Parent->GetVectorParameterValue(_Name, Existing)) return;
    __BaseColor = _Value; __ColorOverride = true; MaterialChanged();
}
void UMaterialInstance::SetScalarParameterValueInternal(FName _Name, float _Value)
{
    std::lock_guard<std::recursive_mutex> MaterialLock(MaterialMutex());
    float Existing;
    if (!std::isfinite(_Value) || !__Parent || !__Parent->GetScalarParameterValue(_Name, Existing)) return;
    __Opacity = std::clamp(_Value, 0.0f, 1.0f); __OpacityOverride = true; MaterialChanged();
}
void UMaterialInstance::ClearParameterValues()
{
    std::lock_guard<std::recursive_mutex> MaterialLock(MaterialMutex());
    __TextureOverride = __ColorOverride = __OpacityOverride = false;
    __Texture = nullptr; __BaseColor = FLinearColor(1, 1, 1, 1); __Opacity = 1;
    MaterialChanged();
}
void UMaterialInstance::Serialize(FArchive& _Archive)
{
    std::lock_guard<std::recursive_mutex> MaterialLock(MaterialMutex());
    UObject* Parent = __Parent;
    _Archive.AssetReference(Parent);
    if (_Archive.IsLoading())
    {
        auto* Material = dynamic_cast<UMaterialInterface*>(Parent);
        if (!Material || !SetParentInternal(Material)) { _Archive.SetError(); return; }
    }
    if (!Parent || dynamic_cast<UMaterialInstanceDynamic*>(Parent)) _Archive.SetError();
    uint32 Flags = (__TextureOverride ? 1u : 0u) | (__ColorOverride ? 2u : 0u) | (__OpacityOverride ? 4u : 0u);
    _Archive.UInt32(Flags);
    if (Flags > 7) { _Archive.SetError(); return; }
    if (_Archive.IsLoading())
    {
        __TextureOverride = (Flags & 1) != 0; __ColorOverride = (Flags & 2) != 0; __OpacityOverride = (Flags & 4) != 0;
    }
    Values(_Archive, __Texture, __BaseColor, __Opacity);
}

UMaterialInstanceDynamic* UMaterialInstanceDynamic::Create(UMaterialInterface* _ParentMaterial, UObject* _Outer, FName _Name)
{
    std::lock_guard<std::recursive_mutex> MaterialLock(MaterialMutex());
    if (!_ParentMaterial) return nullptr;
    if (!_Outer) _Outer = _ParentMaterial;
    auto Instance = std::make_unique<UMaterialInstanceDynamic>();
    if (!Instance->SetParentInternal(_ParentMaterial)) return nullptr;
    Instance->__Outer = _Outer;
    Instance->__ObjectName = _Name.ToString();
    auto* Result = Instance.get();
    _Outer->__OwnedObjects.push_back(std::move(Instance));
    return Result;
}
