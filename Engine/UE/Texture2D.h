#pragma once
#include "Texture.h"
#include "AssetImportData.h"

UCLASS(MinimalAPI)
class ENGINE_API UTexture2D final : public UTexture
{
    GENERATED_BODY()

public:
    uint32 GetSizeX() const override { return __SizeX; }
    uint32 GetSizeY() const override { return __SizeY; }
    const std::vector<uint8>& GetPixels() const { return __Pixels; }
    bool SetPixels(uint32 _SizeX, uint32 _SizeY, std::vector<uint8> _Pixels);
    void Serialize(FArchive& _Archive) override;
    void PostLoad() override;
    UAssetImportData* GetAssetImportData() const { return __AssetImportData.get(); }

private:
    friend class FPackageStore;
    std::unique_ptr<UAssetImportData> __AssetImportData = std::make_unique<UAssetImportData>();
    uint32 __SizeX = 0;
    uint32 __SizeY = 0;
    std::vector<uint8> __Pixels;
};
