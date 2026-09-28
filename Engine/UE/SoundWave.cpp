#include "pch.h"
#include "SoundWave.h"
#include "Archive.h"

float USoundWave::GetDuration() const
{
    return __SampleRate && __NumChannels ? static_cast<float>(__PCMData.size()) / (__SampleRate * __NumChannels * 2) : 0.0f;
}

bool USoundWave::SetPCMData(uint32 _SampleRate, uint32 _NumChannels, std::vector<uint8> _Data)
{
    if (_SampleRate < 8000 || _SampleRate > 192000 || _NumChannels < 1 || _NumChannels > 2 ||
        _Data.empty() || _Data.size() > 256u * 1024 * 1024 || _Data.size() % (_NumChannels * 2)) return false;
    __SampleRate = _SampleRate;
    __NumChannels = _NumChannels;
    __PCMData = std::move(_Data);
    return true;
}

void USoundWave::Serialize(FArchive& _Archive)
{
    _Archive.UInt32(__SampleRate);
    _Archive.UInt32(__NumChannels);
    if (__SampleRate < 8000 || __SampleRate > 192000 || __NumChannels < 1 || __NumChannels > 2) { _Archive.SetError(); return; }
    _Archive.Bytes(__PCMData, 256u * 1024 * 1024);
    if (__PCMData.empty() || __PCMData.size() % (__NumChannels * 2)) _Archive.SetError();
}
