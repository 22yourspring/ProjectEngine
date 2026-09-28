#pragma once
#include "SceneComponent.h"

class USoundBase;

UCLASS(MinimalAPI)
class ENGINE_API UAudioComponent final : public USceneComponent
{
    GENERATED_BODY()

public:
    UAudioComponent();
    ~UAudioComponent() override;
    void SetSound(USoundBase* _Sound);
    USoundBase* GetSound() const { return __Sound; }
    float GetVolumeMultiplier() const { return __Volume; }
    float GetPitchMultiplier() const { return __Pitch; }
    bool IsLooping() const { return __Looping; }
    bool Play();
    void Stop();
    bool IsPlaying() const;
    void SetVolumeMultiplier(float _Volume);
    void SetPitchMultiplier(float _Pitch);
    void SetLooping(bool _Looping) { __Looping = _Looping; }
    void OnUnregister() override;
    void EndPlay() override;

private:
    struct FPlayback;
    std::unique_ptr<FPlayback> __Playback;
    USoundBase* __Sound = nullptr;
    float __Volume = 1.0f;
    float __Pitch = 1.0f;
    bool __Looping = false;
};
