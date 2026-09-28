#pragma once
#include "SoundBase.h"
#include "AssetImportData.h"

UCLASS(MinimalAPI)
class ENGINE_API USoundWave final : public USoundBase
{
    GENERATED_BODY()

public:
    uint32 GetSampleRate() const { return __SampleRate; }
    uint32 GetNumChannels() const { return __NumChannels; }
    const std::vector<uint8>& GetPCMData() const { return __PCMData; }
    float GetDuration() const override;
    bool SetPCMData(uint32 _SampleRate, uint32 _NumChannels, std::vector<uint8> _Data);
    void Serialize(FArchive& _Archive) override;
    UAssetImportData* GetAssetImportData() const { return __AssetImportData.get(); }

private:
    friend class FPackageStore;
    std::unique_ptr<UAssetImportData> __AssetImportData = std::make_unique<UAssetImportData>();
    uint32 __SampleRate = 0;
    uint32 __NumChannels = 0;
    std::vector<uint8> __PCMData;
};
