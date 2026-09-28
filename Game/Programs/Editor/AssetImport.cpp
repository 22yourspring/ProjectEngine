#include "framework.h"
#include "AssetImport.h"
#include "UE/Texture2D.h"
#include "UE/SoundWave.h"
#include <wincodec.h>
#pragma push_macro("Super")
#undef Super
#include <wrl/client.h>
#pragma pop_macro("Super")
#include <fstream>
#include <cstring>
#pragma comment(lib, "windowscodecs.lib")
#pragma comment(lib, "ole32.lib")

namespace
{
    using Microsoft::WRL::ComPtr;

    std::unique_ptr<UTexture2D> ImportTexture(const std::filesystem::path& _Source, FString& _Error)
    {
        const HRESULT ComResult = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
        if (FAILED(ComResult) && ComResult != RPC_E_CHANGED_MODE) { _Error = TEXT("Image decoder initialization failed."); return nullptr; }
        struct FComScope
        {
            bool __Initialized;
            ~FComScope() { if (__Initialized) CoUninitialize(); }
        } Scope { SUCCEEDED(ComResult) };
        ComPtr<IWICImagingFactory> Factory;
        ComPtr<IWICBitmapDecoder> Decoder;
        ComPtr<IWICBitmapFrameDecode> Frame;
        ComPtr<IWICFormatConverter> Converter;
        UINT Width = 0, Height = 0;
        if (FAILED(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(Factory.GetAddressOf()))) ||
            FAILED(Factory->CreateDecoderFromFilename(_Source.c_str(), nullptr, GENERIC_READ, WICDecodeMetadataCacheOnLoad, Decoder.GetAddressOf())) ||
            FAILED(Decoder->GetFrame(0, Frame.GetAddressOf())) || FAILED(Frame->GetSize(&Width, &Height)) ||
            !Width || !Height || Width > 8192 || Height > 8192 ||
            FAILED(Factory->CreateFormatConverter(Converter.GetAddressOf())) ||
            FAILED(Converter->Initialize(Frame.Get(), GUID_WICPixelFormat32bppRGBA, WICBitmapDitherTypeNone, nullptr, 0.0, WICBitmapPaletteTypeCustom)))
        {
            _Error = TEXT("Could not decode image. Use a PNG up to 8192 x 8192 pixels.");
            return nullptr;
        }
        std::vector<uint8> Pixels(static_cast<size_t>(Width) * Height * 4);
        if (FAILED(Converter->CopyPixels(nullptr, Width * 4, static_cast<UINT>(Pixels.size()), Pixels.data())))
        {
            _Error = TEXT("Image pixel data could not be decoded.");
            return nullptr;
        }
        auto Texture = std::make_unique<UTexture2D>();
        if (!Texture->SetPixels(Width, Height, std::move(Pixels))) return nullptr;
        return Texture;
    }

    uint32 Read32(const uint8* _Data)
    {
        return uint32(_Data[0]) | uint32(_Data[1]) << 8 | uint32(_Data[2]) << 16 | uint32(_Data[3]) << 24;
    }

    uint32 Read16(const uint8* _Data)
    {
        return uint32(_Data[0]) | uint32(_Data[1]) << 8;
    }

    std::unique_ptr<USoundWave> ImportSound(const std::filesystem::path& _Source, FString& _Error)
    {
        _Error = TEXT("Use an uncompressed 16-bit PCM WAV, mono or stereo, 8000-192000 Hz (up to 256 MB).");
        std::ifstream Input(_Source, std::ios::binary | std::ios::ate);
        if (!Input) return nullptr;
        const auto Length = Input.tellg();
        if (Length < 44 || Length > 256ull * 1024 * 1024) return nullptr;
        std::vector<uint8> Bytes(static_cast<size_t>(Length));
        Input.seekg(0);
        if (!Input.read(reinterpret_cast<char*>(Bytes.data()), static_cast<std::streamsize>(Bytes.size()))) return nullptr;
        if (std::memcmp(Bytes.data(), "RIFF", 4) || std::memcmp(Bytes.data() + 8, "WAVE", 4) ||
            uint64(Read32(Bytes.data() + 4)) + 8 != Bytes.size()) return nullptr;
        uint32 Rate = 0, Channels = 0;
        bool HasFormat = false, HasData = false;
        std::vector<uint8> Samples;
        size_t Offset = 12;
        while (Offset < Bytes.size())
        {
            if (Bytes.size() - Offset < 8) return nullptr;
            const uint8* Chunk = Bytes.data() + Offset;
            const uint32 Size = Read32(Chunk + 4);
            Offset += 8;
            if (Size > Bytes.size() - Offset) return nullptr;
            const uint8* Data = Bytes.data() + Offset;
            if (!std::memcmp(Chunk, "fmt ", 4))
            {
                if (HasFormat || Size < 16 || Read16(Data) != 1 || Read16(Data + 14) != 16) return nullptr;
                Channels = Read16(Data + 2);
                Rate = Read32(Data + 4);
                if (Channels < 1 || Channels > 2 || Rate < 8000 || Rate > 192000 ||
                    Read16(Data + 12) != Channels * 2 || Read32(Data + 8) != Rate * Channels * 2) return nullptr;
                HasFormat = true;
            }
            else if (!std::memcmp(Chunk, "data", 4))
            {
                if (HasData) return nullptr;
                Samples.assign(Data, Data + Size);
                HasData = true;
            }
            Offset += Size;
            if (Size & 1) { if (Offset == Bytes.size()) return nullptr; ++Offset; }
        }
        auto Wave = std::make_unique<USoundWave>();
        if (!HasFormat || !HasData || !Wave->SetPCMData(Rate, Channels, std::move(Samples))) return nullptr;
        _Error.Empty();
        return Wave;
    }
}

std::unique_ptr<UObject> UTextureFactory::FactoryCreateFile(const std::filesystem::path& _Source, FString& _Error)
{
    return ImportTexture(_Source, _Error);
}

std::unique_ptr<UObject> USoundFactory::FactoryCreateFile(const std::filesystem::path& _Source, FString& _Error)
{
    return ImportSound(_Source, _Error);
}

bool ImportAsset(FPackageStore& _Packages, const std::filesystem::path& _Source,
    const FString& _ObjectPath, FString& _Error)
{
    _Error.Empty();
    try
    {
        std::unique_ptr<UObject> Object;
        const auto Extension = _Source.extension().wstring();
        if (!_wcsicmp(Extension.c_str(), L".png")) Object = UTextureFactory().FactoryCreateFile(_Source, _Error);
        else if (!_wcsicmp(Extension.c_str(), L".wav")) Object = USoundFactory().FactoryCreateFile(_Source, _Error);
        else { _Error = TEXT("Select a PNG image or a WAV sound."); return false; }
        if (!Object) return false;
        std::filesystem::path File;
        FString Package, Name;
        if (!_Packages.Resolve(_ObjectPath, File, Package, Name)) { _Error = TEXT("Invalid destination asset path."); return false; }
        std::error_code Error;
        auto Relative = std::filesystem::relative(std::filesystem::absolute(_Source), File.parent_path(), Error);
        return _Packages.Save(_ObjectPath, *Object,
            FString((Error || Relative.empty() ? std::filesystem::absolute(_Source) : Relative).wstring()), _Error);
    }
    catch (const std::exception&) { _Error = TEXT("Import failed while reading the source file."); return false; }
}
