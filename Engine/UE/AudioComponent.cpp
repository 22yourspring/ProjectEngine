#include "pch.h"
#include "AudioComponent.h"
#include "SoundWave.h"
#include <xaudio2.h>
#include <cmath>
#include <algorithm>
#pragma comment(lib, "xaudio2.lib")
#pragma comment(lib, "ole32.lib")

struct UAudioComponent::FPlayback
{
    IXAudio2* __Engine = nullptr;
    IXAudio2MasteringVoice* __Master = nullptr;
    IXAudio2SourceVoice* __Voice = nullptr;
    std::vector<uint8> __Data;

    ~FPlayback()
    {
        if (__Voice) __Voice->DestroyVoice();
        if (__Master) __Master->DestroyVoice();
        if (__Engine) __Engine->Release();
    }
};

UAudioComponent::UAudioComponent() = default;
UAudioComponent::~UAudioComponent() = default;

void UAudioComponent::SetSound(USoundBase* _Sound)
{
    Stop();
    __Sound = _Sound;
}

bool UAudioComponent::Play()
{
    Stop();
    const auto* Wave = dynamic_cast<const USoundWave*>(__Sound);
    if (!Wave || Wave->GetPCMData().empty()) return false;
    auto Playback = std::make_unique<FPlayback>();
    if (FAILED(XAudio2Create(&Playback->__Engine)) || FAILED(Playback->__Engine->CreateMasteringVoice(&Playback->__Master))) return false;
    WAVEFORMATEX Format = {};
    Format.wFormatTag = WAVE_FORMAT_PCM;
    Format.nChannels = static_cast<WORD>(Wave->GetNumChannels());
    Format.nSamplesPerSec = Wave->GetSampleRate();
    Format.wBitsPerSample = 16;
    Format.nBlockAlign = Format.nChannels * 2;
    Format.nAvgBytesPerSec = Format.nSamplesPerSec * Format.nBlockAlign;
    if (FAILED(Playback->__Engine->CreateSourceVoice(&Playback->__Voice, &Format, 0, 4.0f))) return false;
    Playback->__Data = Wave->GetPCMData();
    XAUDIO2_BUFFER Buffer = {};
    Buffer.Flags = XAUDIO2_END_OF_STREAM;
    Buffer.AudioBytes = static_cast<UINT32>(Playback->__Data.size());
    Buffer.pAudioData = Playback->__Data.data();
    Buffer.LoopCount = __Looping ? XAUDIO2_LOOP_INFINITE : 0;
    if (FAILED(Playback->__Voice->SetVolume(__Volume)) || FAILED(Playback->__Voice->SetFrequencyRatio(__Pitch)) ||
        FAILED(Playback->__Voice->SubmitSourceBuffer(&Buffer)) || FAILED(Playback->__Voice->Start())) return false;
    __Playback = std::move(Playback);
    return true;
}

void UAudioComponent::Stop()
{
    __Playback.reset();
}

bool UAudioComponent::IsPlaying() const
{
    if (!__Playback) return false;
    XAUDIO2_VOICE_STATE State = {};
    __Playback->__Voice->GetState(&State, XAUDIO2_VOICE_NOSAMPLESPLAYED);
    return State.BuffersQueued != 0;
}

void UAudioComponent::SetVolumeMultiplier(float _Volume)
{
    if (!std::isfinite(_Volume)) return;
    __Volume = (std::clamp)(_Volume, 0.0f, 4.0f);
    if (__Playback) __Playback->__Voice->SetVolume(__Volume);
}

void UAudioComponent::SetPitchMultiplier(float _Pitch)
{
    if (!std::isfinite(_Pitch)) return;
    __Pitch = (std::clamp)(_Pitch, 0.25f, 4.0f);
    if (__Playback) __Playback->__Voice->SetFrequencyRatio(__Pitch);
}

void UAudioComponent::OnUnregister()
{
    Stop();
    USceneComponent::OnUnregister();
}

void UAudioComponent::EndPlay()
{
    Stop();
    USceneComponent::EndPlay();
}
