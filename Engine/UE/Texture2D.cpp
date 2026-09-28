#include "pch.h"
#include "Texture2D.h"
#include "Archive.h"
#include "MaterialInterface.h"

void UTexture2D::PostLoad() { UMaterialInterface::RefreshAllMaterials(); }

bool UTexture2D::SetPixels(uint32 _SizeX, uint32 _SizeY, std::vector<uint8> _Pixels)
{
    if (!_SizeX || !_SizeY || _SizeX > 8192 || _SizeY > 8192 || uint64(_SizeX) * _SizeY * 4 != _Pixels.size()) return false;
    __SizeX = _SizeX;
    __SizeY = _SizeY;
    __Pixels = std::move(_Pixels);
    UMaterialInterface::RefreshAllMaterials();
    return true;
}

void UTexture2D::Serialize(FArchive& _Archive)
{
    _Archive.UInt32(__SizeX);
    _Archive.UInt32(__SizeY);
    if (!__SizeX || !__SizeY || __SizeX > 8192 || __SizeY > 8192) { _Archive.SetError(); return; }
    _Archive.Bytes(__Pixels, __SizeX * __SizeY * 4);
    if (__Pixels.size() != uint64(__SizeX) * __SizeY * 4) _Archive.SetError();
}
