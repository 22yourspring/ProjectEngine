#include "pch.h"
#include "KismetSystemLibrary.h"
#include "World.h"
#include "PrimitiveComponent.h"
#include "CollisionProfile.h"
#include "Scene.h"
#include <cmath>

namespace
{
    std::vector<ECollisionChannel> QueryChannels(bool _Trace)
    {
        std::vector<ECollisionChannel> Result;
        for (const auto& Channel : UCollisionProfile::Get()->GetChannels())
            if (Channel.bTraceType == _Trace) Result.push_back(Channel.Channel);
        std::sort(Result.begin(), Result.end());
        return Result;
    }
    FCollisionObjectQueryParams ObjectParams(const std::vector<EObjectTypeQuery>& _Types)
    {
        FCollisionObjectQueryParams Result;
        for (const auto Type : _Types) Result.AddObjectTypesToQuery(UEngineTypes::ConvertToCollisionChannel(Type));
        return Result;
    }
    FCollisionQueryParams QueryParams(const UObject* _Context, bool _Complex, const std::vector<AActor*>& _Ignored, bool _IgnoreSelf)
    {
        FCollisionQueryParams Result; Result.bTraceComplex = _Complex;
        for (auto* Actor : _Ignored) Result.AddIgnoredActor(Actor);
        if (_IgnoreSelf)
        {
            for (auto* Object = _Context; Object; Object = Object->GetOuter())
            {
                if (auto* Actor = dynamic_cast<const AActor*>(Object)) { Result.AddIgnoredActor(Actor); break; }
                if (auto* Component = dynamic_cast<const UActorComponent*>(Object)) { Result.AddIgnoredActor(Component->GetOwner()); break; }
            }
        }
        return Result;
    }
    void DrawTrace(UWorld* _World, const FVector& _Start, const FVector& _End, const FCollisionShape* _Shape, const FQuat& _Rot,
        EDrawDebugTrace::Type _Mode, const std::vector<FHitResult>& _Hits, FLinearColor _Color, FLinearColor _HitColor, float _Time)
    {
        if (!_World || _Mode == EDrawDebugTrace::None) return;
        auto* Scene = _World->GetScene();
        const float Duration = _Mode == EDrawDebugTrace::ForDuration && std::isfinite(_Time) ? (std::max)(0.f, _Time) : 0.f;
        auto Line = [&](FVector _A, FVector _B, FLinearColor _Tint)
        {
            auto Channel = [](float _Value)
            {
                if (!std::isfinite(_Value)) return uint8(0);
                _Value = std::clamp(_Value, 0.f, 1.f);
                const float SRGB = _Value <= 0.0031308f ? 12.92f * _Value : 1.055f * std::pow(_Value, 1.f / 2.4f) - 0.055f;
                return uint8(std::round(SRGB * 255.f));
            };
            Scene->AddDebugLine(_A, _B, FColor(Channel(_Tint.R), Channel(_Tint.G), Channel(_Tint.B)), Duration, _Mode == EDrawDebugTrace::Persistent);
        };
        auto Block = std::find_if(_Hits.begin(), _Hits.end(), [](const auto& _Hit) { return _Hit.bBlockingHit; });
        const FVector Split = Block != _Hits.end() ? Block->Location : _End;
        Line(_Start, Split, _Color);
        if (Block != _Hits.end()) Line(Split, _End, _HitColor);
        if (_Shape)
        {
            auto ShapeAt = [&](FVector _Position, FLinearColor _Tint)
            {
                auto Edge = [&](FVector _A, FVector _B) { Line(_Position + _Rot.RotateVector(_A), _Position + _Rot.RotateVector(_B), _Tint); };
                if (_Shape->IsBox())
                {
                    const auto E = _Shape->GetBox();
                    for (int Corner = 0; Corner < 8; ++Corner)
                        for (int Axis = 0; Axis < 3; ++Axis)
                            if (!(Corner & (1 << Axis)))
                            {
                                const int Other = Corner | (1 << Axis);
                                Edge(FVector(Corner & 1 ? E.X : -E.X, Corner & 2 ? E.Y : -E.Y, Corner & 4 ? E.Z : -E.Z),
                                    FVector(Other & 1 ? E.X : -E.X, Other & 2 ? E.Y : -E.Y, Other & 4 ? E.Z : -E.Z));
                            }
                }
                else
                {
                    const double R = _Shape->IsSphere() ? _Shape->GetSphereRadius() : _Shape->GetCapsuleRadius();
                    const double H = _Shape->IsCapsule() ? _Shape->GetCapsuleHalfHeight() - R : 0;
                    for (int Plane = 0; Plane < 3; ++Plane)
                        for (int Index = 0; Index < 32; ++Index)
                        {
                            auto Point = [&](int _Index)
                            {
                                const double Angle = _Index * 6.283185307179586 / 32;
                                const double A = R * std::cos(Angle), B = R * std::sin(Angle);
                                if (Plane == 0) return FVector(A, B, H);
                                const double Z = B + (B >= 0 ? H : -H);
                                return Plane == 1 ? FVector(A, 0, Z) : FVector(0, A, Z);
                            };
                            Edge(Point(Index), Point(Index + 1));
                            if (Plane == 0 && H > 0) Edge(Point(Index) - FVector(0, 0, 2 * H), Point(Index + 1) - FVector(0, 0, 2 * H));
                        }
                }
            };
            ShapeAt(_Start, _Color); ShapeAt(Split, Block != _Hits.end() ? _HitColor : _Color);
        }
        for (const auto& Hit : _Hits)
        {
            Line(Hit.ImpactPoint - FVector(3, 0, 0), Hit.ImpactPoint + FVector(3, 0, 0), _HitColor);
            Line(Hit.ImpactPoint - FVector(0, 3, 0), Hit.ImpactPoint + FVector(0, 3, 0), _HitColor);
        }
    }
}
ECollisionChannel UEngineTypes::ConvertToCollisionChannel(ETraceTypeQuery _TraceType)
{
    const auto Channels = QueryChannels(true);
    return int(_TraceType) >= 0 && size_t(_TraceType) < Channels.size() ? Channels[size_t(_TraceType)] : ECC_MAX;
}
ECollisionChannel UEngineTypes::ConvertToCollisionChannel(EObjectTypeQuery _ObjectType)
{
    const auto Channels = QueryChannels(false);
    return int(_ObjectType) >= 0 && size_t(_ObjectType) < Channels.size() ? Channels[size_t(_ObjectType)] : ECC_MAX;
}
ETraceTypeQuery UEngineTypes::ConvertToTraceType(ECollisionChannel _Channel)
{
    const auto Channels = QueryChannels(true);
    const auto Found = std::find(Channels.begin(), Channels.end(), _Channel);
    return Found == Channels.end() ? ETraceTypeQuery_MAX : ETraceTypeQuery(Found - Channels.begin());
}
EObjectTypeQuery UEngineTypes::ConvertToObjectType(ECollisionChannel _Channel)
{
    const auto Channels = QueryChannels(false);
    const auto Found = std::find(Channels.begin(), Channels.end(), _Channel);
    return Found == Channels.end() ? EObjectTypeQuery_MAX : EObjectTypeQuery(Found - Channels.begin());
}
void UKismetSystemLibrary::FlushPersistentDebugLines(const UObject* _WorldContextObject)
{
    if (auto* World = _WorldContextObject ? _WorldContextObject->GetWorld() : nullptr) World->GetScene()->FlushDebugLines();
}
bool UKismetSystemLibrary::LineTraceSingle(const UObject* _WorldContextObject, const FVector _Start, const FVector _End, ETraceTypeQuery _TraceChannel, bool _bTraceComplex, const std::vector<AActor*>& _ActorsToIgnore, EDrawDebugTrace::Type _DrawDebugType, FHitResult& _OutHit, bool _bIgnoreSelf, FLinearColor _TraceColor, FLinearColor _TraceHitColor, float _DrawTime)
{
    _OutHit = {}; _OutHit.TraceStart = _Start; _OutHit.TraceEnd = _End;
    auto* World = _WorldContextObject ? _WorldContextObject->GetWorld() : nullptr;
    if (!World) return false;
    const auto Params = QueryParams(_WorldContextObject, _bTraceComplex, _ActorsToIgnore, _bIgnoreSelf);
    const bool Result = World->LineTraceSingleByChannel(_OutHit, _Start, _End, UEngineTypes::ConvertToCollisionChannel(_TraceChannel), Params);
    DrawTrace(World, _Start, _End, nullptr, FQuat::Identity, _DrawDebugType, Result ? std::vector<FHitResult>{_OutHit} : std::vector<FHitResult>{}, _TraceColor, _TraceHitColor, _DrawTime);
    return Result;
}
bool UKismetSystemLibrary::LineTraceMulti(const UObject* _WorldContextObject, const FVector _Start, const FVector _End, ETraceTypeQuery _TraceChannel, bool _bTraceComplex, const std::vector<AActor*>& _ActorsToIgnore, EDrawDebugTrace::Type _DrawDebugType, std::vector<FHitResult>& _OutHits, bool _bIgnoreSelf, FLinearColor _TraceColor, FLinearColor _TraceHitColor, float _DrawTime)
{
    _OutHits.clear();
    auto* World = _WorldContextObject ? _WorldContextObject->GetWorld() : nullptr;
    if (!World) return false;
    const auto Params = QueryParams(_WorldContextObject, _bTraceComplex, _ActorsToIgnore, _bIgnoreSelf);
    const bool Result = World->LineTraceMultiByChannel(_OutHits, _Start, _End, UEngineTypes::ConvertToCollisionChannel(_TraceChannel), Params);
    DrawTrace(World, _Start, _End, nullptr, FQuat::Identity, _DrawDebugType, _OutHits, _TraceColor, _TraceHitColor, _DrawTime);
    return Result;
}
bool UKismetSystemLibrary::LineTraceSingleForObjects(const UObject* _WorldContextObject, const FVector _Start, const FVector _End, const std::vector<EObjectTypeQuery>& _ObjectTypes, bool _bTraceComplex, const std::vector<AActor*>& _ActorsToIgnore, EDrawDebugTrace::Type _DrawDebugType, FHitResult& _OutHit, bool _bIgnoreSelf, FLinearColor _TraceColor, FLinearColor _TraceHitColor, float _DrawTime)
{
    _OutHit = {}; _OutHit.TraceStart = _Start; _OutHit.TraceEnd = _End;
    auto* World = _WorldContextObject ? _WorldContextObject->GetWorld() : nullptr;
    if (!World) return false;
    const auto Params = QueryParams(_WorldContextObject, _bTraceComplex, _ActorsToIgnore, _bIgnoreSelf);
    const bool Result = World->LineTraceSingleByObjectType(_OutHit, _Start, _End, ObjectParams(_ObjectTypes), Params);
    DrawTrace(World, _Start, _End, nullptr, FQuat::Identity, _DrawDebugType, Result ? std::vector<FHitResult>{_OutHit} : std::vector<FHitResult>{}, _TraceColor, _TraceHitColor, _DrawTime);
    return Result;
}
bool UKismetSystemLibrary::LineTraceMultiForObjects(const UObject* _WorldContextObject, const FVector _Start, const FVector _End, const std::vector<EObjectTypeQuery>& _ObjectTypes, bool _bTraceComplex, const std::vector<AActor*>& _ActorsToIgnore, EDrawDebugTrace::Type _DrawDebugType, std::vector<FHitResult>& _OutHits, bool _bIgnoreSelf, FLinearColor _TraceColor, FLinearColor _TraceHitColor, float _DrawTime)
{
    _OutHits.clear();
    auto* World = _WorldContextObject ? _WorldContextObject->GetWorld() : nullptr;
    if (!World) return false;
    const auto Params = QueryParams(_WorldContextObject, _bTraceComplex, _ActorsToIgnore, _bIgnoreSelf);
    const bool Result = World->LineTraceMultiByObjectType(_OutHits, _Start, _End, ObjectParams(_ObjectTypes), Params);
    DrawTrace(World, _Start, _End, nullptr, FQuat::Identity, _DrawDebugType, _OutHits, _TraceColor, _TraceHitColor, _DrawTime);
    return Result;
}
bool UKismetSystemLibrary::LineTraceSingleByProfile(const UObject* _WorldContextObject, const FVector _Start, const FVector _End, FName _ProfileName, bool _bTraceComplex, const std::vector<AActor*>& _ActorsToIgnore, EDrawDebugTrace::Type _DrawDebugType, FHitResult& _OutHit, bool _bIgnoreSelf, FLinearColor _TraceColor, FLinearColor _TraceHitColor, float _DrawTime)
{
    _OutHit = {}; _OutHit.TraceStart = _Start; _OutHit.TraceEnd = _End;
    auto* World = _WorldContextObject ? _WorldContextObject->GetWorld() : nullptr;
    if (!World) return false;
    const auto Params = QueryParams(_WorldContextObject, _bTraceComplex, _ActorsToIgnore, _bIgnoreSelf);
    const bool Result = World->LineTraceSingleByProfile(_OutHit, _Start, _End, _ProfileName, Params);
    DrawTrace(World, _Start, _End, nullptr, FQuat::Identity, _DrawDebugType, Result ? std::vector<FHitResult>{_OutHit} : std::vector<FHitResult>{}, _TraceColor, _TraceHitColor, _DrawTime);
    return Result;
}
bool UKismetSystemLibrary::LineTraceMultiByProfile(const UObject* _WorldContextObject, const FVector _Start, const FVector _End, FName _ProfileName, bool _bTraceComplex, const std::vector<AActor*>& _ActorsToIgnore, EDrawDebugTrace::Type _DrawDebugType, std::vector<FHitResult>& _OutHits, bool _bIgnoreSelf, FLinearColor _TraceColor, FLinearColor _TraceHitColor, float _DrawTime)
{
    _OutHits.clear();
    auto* World = _WorldContextObject ? _WorldContextObject->GetWorld() : nullptr;
    if (!World) return false;
    const auto Params = QueryParams(_WorldContextObject, _bTraceComplex, _ActorsToIgnore, _bIgnoreSelf);
    const bool Result = World->LineTraceMultiByProfile(_OutHits, _Start, _End, _ProfileName, Params);
    DrawTrace(World, _Start, _End, nullptr, FQuat::Identity, _DrawDebugType, _OutHits, _TraceColor, _TraceHitColor, _DrawTime);
    return Result;
}
bool UKismetSystemLibrary::BoxTraceSingle(const UObject* _WorldContextObject, const FVector _Start, const FVector _End, const FVector _HalfSize, const FRotator _Orientation, ETraceTypeQuery _TraceChannel, bool _bTraceComplex, const std::vector<AActor*>& _ActorsToIgnore, EDrawDebugTrace::Type _DrawDebugType, FHitResult& _OutHit, bool _bIgnoreSelf, FLinearColor _TraceColor, FLinearColor _TraceHitColor, float _DrawTime)
{
    _OutHit = {}; _OutHit.TraceStart = _Start; _OutHit.TraceEnd = _End;
    auto* World = _WorldContextObject ? _WorldContextObject->GetWorld() : nullptr;
    if (!World) return false;
    const auto Params = QueryParams(_WorldContextObject, _bTraceComplex, _ActorsToIgnore, _bIgnoreSelf);
    const auto Shape = FCollisionShape::MakeBox(_HalfSize);
    const auto Rotation = _Orientation.Quaternion();
    const bool Result = World->SweepSingleByChannel(_OutHit, _Start, _End, Rotation, UEngineTypes::ConvertToCollisionChannel(_TraceChannel), Shape, Params);
    DrawTrace(World, _Start, _End, &Shape, Rotation, _DrawDebugType, Result ? std::vector<FHitResult>{_OutHit} : std::vector<FHitResult>{}, _TraceColor, _TraceHitColor, _DrawTime);
    return Result;
}
bool UKismetSystemLibrary::BoxTraceMulti(const UObject* _WorldContextObject, const FVector _Start, const FVector _End, const FVector _HalfSize, const FRotator _Orientation, ETraceTypeQuery _TraceChannel, bool _bTraceComplex, const std::vector<AActor*>& _ActorsToIgnore, EDrawDebugTrace::Type _DrawDebugType, std::vector<FHitResult>& _OutHits, bool _bIgnoreSelf, FLinearColor _TraceColor, FLinearColor _TraceHitColor, float _DrawTime)
{
    _OutHits.clear();
    auto* World = _WorldContextObject ? _WorldContextObject->GetWorld() : nullptr;
    if (!World) return false;
    const auto Params = QueryParams(_WorldContextObject, _bTraceComplex, _ActorsToIgnore, _bIgnoreSelf);
    const auto Shape = FCollisionShape::MakeBox(_HalfSize);
    const auto Rotation = _Orientation.Quaternion();
    const bool Result = World->SweepMultiByChannel(_OutHits, _Start, _End, Rotation, UEngineTypes::ConvertToCollisionChannel(_TraceChannel), Shape, Params);
    DrawTrace(World, _Start, _End, &Shape, Rotation, _DrawDebugType, _OutHits, _TraceColor, _TraceHitColor, _DrawTime);
    return Result;
}
bool UKismetSystemLibrary::BoxTraceSingleForObjects(const UObject* _WorldContextObject, const FVector _Start, const FVector _End, const FVector _HalfSize, const FRotator _Orientation, const std::vector<EObjectTypeQuery>& _ObjectTypes, bool _bTraceComplex, const std::vector<AActor*>& _ActorsToIgnore, EDrawDebugTrace::Type _DrawDebugType, FHitResult& _OutHit, bool _bIgnoreSelf, FLinearColor _TraceColor, FLinearColor _TraceHitColor, float _DrawTime)
{
    _OutHit = {}; _OutHit.TraceStart = _Start; _OutHit.TraceEnd = _End;
    auto* World = _WorldContextObject ? _WorldContextObject->GetWorld() : nullptr;
    if (!World) return false;
    const auto Params = QueryParams(_WorldContextObject, _bTraceComplex, _ActorsToIgnore, _bIgnoreSelf);
    const auto Shape = FCollisionShape::MakeBox(_HalfSize);
    const auto Rotation = _Orientation.Quaternion();
    const bool Result = World->SweepSingleByObjectType(_OutHit, _Start, _End, Rotation, ObjectParams(_ObjectTypes), Shape, Params);
    DrawTrace(World, _Start, _End, &Shape, Rotation, _DrawDebugType, Result ? std::vector<FHitResult>{_OutHit} : std::vector<FHitResult>{}, _TraceColor, _TraceHitColor, _DrawTime);
    return Result;
}
bool UKismetSystemLibrary::BoxTraceMultiForObjects(const UObject* _WorldContextObject, const FVector _Start, const FVector _End, const FVector _HalfSize, const FRotator _Orientation, const std::vector<EObjectTypeQuery>& _ObjectTypes, bool _bTraceComplex, const std::vector<AActor*>& _ActorsToIgnore, EDrawDebugTrace::Type _DrawDebugType, std::vector<FHitResult>& _OutHits, bool _bIgnoreSelf, FLinearColor _TraceColor, FLinearColor _TraceHitColor, float _DrawTime)
{
    _OutHits.clear();
    auto* World = _WorldContextObject ? _WorldContextObject->GetWorld() : nullptr;
    if (!World) return false;
    const auto Params = QueryParams(_WorldContextObject, _bTraceComplex, _ActorsToIgnore, _bIgnoreSelf);
    const auto Shape = FCollisionShape::MakeBox(_HalfSize);
    const auto Rotation = _Orientation.Quaternion();
    const bool Result = World->SweepMultiByObjectType(_OutHits, _Start, _End, Rotation, ObjectParams(_ObjectTypes), Shape, Params);
    DrawTrace(World, _Start, _End, &Shape, Rotation, _DrawDebugType, _OutHits, _TraceColor, _TraceHitColor, _DrawTime);
    return Result;
}
bool UKismetSystemLibrary::BoxTraceSingleByProfile(const UObject* _WorldContextObject, const FVector _Start, const FVector _End, const FVector _HalfSize, const FRotator _Orientation, FName _ProfileName, bool _bTraceComplex, const std::vector<AActor*>& _ActorsToIgnore, EDrawDebugTrace::Type _DrawDebugType, FHitResult& _OutHit, bool _bIgnoreSelf, FLinearColor _TraceColor, FLinearColor _TraceHitColor, float _DrawTime)
{
    _OutHit = {}; _OutHit.TraceStart = _Start; _OutHit.TraceEnd = _End;
    auto* World = _WorldContextObject ? _WorldContextObject->GetWorld() : nullptr;
    if (!World) return false;
    const auto Params = QueryParams(_WorldContextObject, _bTraceComplex, _ActorsToIgnore, _bIgnoreSelf);
    const auto Shape = FCollisionShape::MakeBox(_HalfSize);
    const auto Rotation = _Orientation.Quaternion();
    const bool Result = World->SweepSingleByProfile(_OutHit, _Start, _End, Rotation, _ProfileName, Shape, Params);
    DrawTrace(World, _Start, _End, &Shape, Rotation, _DrawDebugType, Result ? std::vector<FHitResult>{_OutHit} : std::vector<FHitResult>{}, _TraceColor, _TraceHitColor, _DrawTime);
    return Result;
}
bool UKismetSystemLibrary::BoxTraceMultiByProfile(const UObject* _WorldContextObject, const FVector _Start, const FVector _End, const FVector _HalfSize, const FRotator _Orientation, FName _ProfileName, bool _bTraceComplex, const std::vector<AActor*>& _ActorsToIgnore, EDrawDebugTrace::Type _DrawDebugType, std::vector<FHitResult>& _OutHits, bool _bIgnoreSelf, FLinearColor _TraceColor, FLinearColor _TraceHitColor, float _DrawTime)
{
    _OutHits.clear();
    auto* World = _WorldContextObject ? _WorldContextObject->GetWorld() : nullptr;
    if (!World) return false;
    const auto Params = QueryParams(_WorldContextObject, _bTraceComplex, _ActorsToIgnore, _bIgnoreSelf);
    const auto Shape = FCollisionShape::MakeBox(_HalfSize);
    const auto Rotation = _Orientation.Quaternion();
    const bool Result = World->SweepMultiByProfile(_OutHits, _Start, _End, Rotation, _ProfileName, Shape, Params);
    DrawTrace(World, _Start, _End, &Shape, Rotation, _DrawDebugType, _OutHits, _TraceColor, _TraceHitColor, _DrawTime);
    return Result;
}
bool UKismetSystemLibrary::SphereTraceSingle(const UObject* _WorldContextObject, const FVector _Start, const FVector _End, float _Radius, ETraceTypeQuery _TraceChannel, bool _bTraceComplex, const std::vector<AActor*>& _ActorsToIgnore, EDrawDebugTrace::Type _DrawDebugType, FHitResult& _OutHit, bool _bIgnoreSelf, FLinearColor _TraceColor, FLinearColor _TraceHitColor, float _DrawTime)
{
    _OutHit = {}; _OutHit.TraceStart = _Start; _OutHit.TraceEnd = _End;
    auto* World = _WorldContextObject ? _WorldContextObject->GetWorld() : nullptr;
    if (!World) return false;
    const auto Params = QueryParams(_WorldContextObject, _bTraceComplex, _ActorsToIgnore, _bIgnoreSelf);
    const auto Shape = FCollisionShape::MakeSphere(_Radius);
    const auto Rotation = FQuat::Identity;
    const bool Result = World->SweepSingleByChannel(_OutHit, _Start, _End, Rotation, UEngineTypes::ConvertToCollisionChannel(_TraceChannel), Shape, Params);
    DrawTrace(World, _Start, _End, &Shape, Rotation, _DrawDebugType, Result ? std::vector<FHitResult>{_OutHit} : std::vector<FHitResult>{}, _TraceColor, _TraceHitColor, _DrawTime);
    return Result;
}
bool UKismetSystemLibrary::SphereTraceMulti(const UObject* _WorldContextObject, const FVector _Start, const FVector _End, float _Radius, ETraceTypeQuery _TraceChannel, bool _bTraceComplex, const std::vector<AActor*>& _ActorsToIgnore, EDrawDebugTrace::Type _DrawDebugType, std::vector<FHitResult>& _OutHits, bool _bIgnoreSelf, FLinearColor _TraceColor, FLinearColor _TraceHitColor, float _DrawTime)
{
    _OutHits.clear();
    auto* World = _WorldContextObject ? _WorldContextObject->GetWorld() : nullptr;
    if (!World) return false;
    const auto Params = QueryParams(_WorldContextObject, _bTraceComplex, _ActorsToIgnore, _bIgnoreSelf);
    const auto Shape = FCollisionShape::MakeSphere(_Radius);
    const auto Rotation = FQuat::Identity;
    const bool Result = World->SweepMultiByChannel(_OutHits, _Start, _End, Rotation, UEngineTypes::ConvertToCollisionChannel(_TraceChannel), Shape, Params);
    DrawTrace(World, _Start, _End, &Shape, Rotation, _DrawDebugType, _OutHits, _TraceColor, _TraceHitColor, _DrawTime);
    return Result;
}
bool UKismetSystemLibrary::SphereTraceSingleForObjects(const UObject* _WorldContextObject, const FVector _Start, const FVector _End, float _Radius, const std::vector<EObjectTypeQuery>& _ObjectTypes, bool _bTraceComplex, const std::vector<AActor*>& _ActorsToIgnore, EDrawDebugTrace::Type _DrawDebugType, FHitResult& _OutHit, bool _bIgnoreSelf, FLinearColor _TraceColor, FLinearColor _TraceHitColor, float _DrawTime)
{
    _OutHit = {}; _OutHit.TraceStart = _Start; _OutHit.TraceEnd = _End;
    auto* World = _WorldContextObject ? _WorldContextObject->GetWorld() : nullptr;
    if (!World) return false;
    const auto Params = QueryParams(_WorldContextObject, _bTraceComplex, _ActorsToIgnore, _bIgnoreSelf);
    const auto Shape = FCollisionShape::MakeSphere(_Radius);
    const auto Rotation = FQuat::Identity;
    const bool Result = World->SweepSingleByObjectType(_OutHit, _Start, _End, Rotation, ObjectParams(_ObjectTypes), Shape, Params);
    DrawTrace(World, _Start, _End, &Shape, Rotation, _DrawDebugType, Result ? std::vector<FHitResult>{_OutHit} : std::vector<FHitResult>{}, _TraceColor, _TraceHitColor, _DrawTime);
    return Result;
}
bool UKismetSystemLibrary::SphereTraceMultiForObjects(const UObject* _WorldContextObject, const FVector _Start, const FVector _End, float _Radius, const std::vector<EObjectTypeQuery>& _ObjectTypes, bool _bTraceComplex, const std::vector<AActor*>& _ActorsToIgnore, EDrawDebugTrace::Type _DrawDebugType, std::vector<FHitResult>& _OutHits, bool _bIgnoreSelf, FLinearColor _TraceColor, FLinearColor _TraceHitColor, float _DrawTime)
{
    _OutHits.clear();
    auto* World = _WorldContextObject ? _WorldContextObject->GetWorld() : nullptr;
    if (!World) return false;
    const auto Params = QueryParams(_WorldContextObject, _bTraceComplex, _ActorsToIgnore, _bIgnoreSelf);
    const auto Shape = FCollisionShape::MakeSphere(_Radius);
    const auto Rotation = FQuat::Identity;
    const bool Result = World->SweepMultiByObjectType(_OutHits, _Start, _End, Rotation, ObjectParams(_ObjectTypes), Shape, Params);
    DrawTrace(World, _Start, _End, &Shape, Rotation, _DrawDebugType, _OutHits, _TraceColor, _TraceHitColor, _DrawTime);
    return Result;
}
bool UKismetSystemLibrary::SphereTraceSingleByProfile(const UObject* _WorldContextObject, const FVector _Start, const FVector _End, float _Radius, FName _ProfileName, bool _bTraceComplex, const std::vector<AActor*>& _ActorsToIgnore, EDrawDebugTrace::Type _DrawDebugType, FHitResult& _OutHit, bool _bIgnoreSelf, FLinearColor _TraceColor, FLinearColor _TraceHitColor, float _DrawTime)
{
    _OutHit = {}; _OutHit.TraceStart = _Start; _OutHit.TraceEnd = _End;
    auto* World = _WorldContextObject ? _WorldContextObject->GetWorld() : nullptr;
    if (!World) return false;
    const auto Params = QueryParams(_WorldContextObject, _bTraceComplex, _ActorsToIgnore, _bIgnoreSelf);
    const auto Shape = FCollisionShape::MakeSphere(_Radius);
    const auto Rotation = FQuat::Identity;
    const bool Result = World->SweepSingleByProfile(_OutHit, _Start, _End, Rotation, _ProfileName, Shape, Params);
    DrawTrace(World, _Start, _End, &Shape, Rotation, _DrawDebugType, Result ? std::vector<FHitResult>{_OutHit} : std::vector<FHitResult>{}, _TraceColor, _TraceHitColor, _DrawTime);
    return Result;
}
bool UKismetSystemLibrary::SphereTraceMultiByProfile(const UObject* _WorldContextObject, const FVector _Start, const FVector _End, float _Radius, FName _ProfileName, bool _bTraceComplex, const std::vector<AActor*>& _ActorsToIgnore, EDrawDebugTrace::Type _DrawDebugType, std::vector<FHitResult>& _OutHits, bool _bIgnoreSelf, FLinearColor _TraceColor, FLinearColor _TraceHitColor, float _DrawTime)
{
    _OutHits.clear();
    auto* World = _WorldContextObject ? _WorldContextObject->GetWorld() : nullptr;
    if (!World) return false;
    const auto Params = QueryParams(_WorldContextObject, _bTraceComplex, _ActorsToIgnore, _bIgnoreSelf);
    const auto Shape = FCollisionShape::MakeSphere(_Radius);
    const auto Rotation = FQuat::Identity;
    const bool Result = World->SweepMultiByProfile(_OutHits, _Start, _End, Rotation, _ProfileName, Shape, Params);
    DrawTrace(World, _Start, _End, &Shape, Rotation, _DrawDebugType, _OutHits, _TraceColor, _TraceHitColor, _DrawTime);
    return Result;
}
bool UKismetSystemLibrary::CapsuleTraceSingle(const UObject* _WorldContextObject, const FVector _Start, const FVector _End, float _Radius, float _HalfHeight, ETraceTypeQuery _TraceChannel, bool _bTraceComplex, const std::vector<AActor*>& _ActorsToIgnore, EDrawDebugTrace::Type _DrawDebugType, FHitResult& _OutHit, bool _bIgnoreSelf, FLinearColor _TraceColor, FLinearColor _TraceHitColor, float _DrawTime)
{
    _OutHit = {}; _OutHit.TraceStart = _Start; _OutHit.TraceEnd = _End;
    auto* World = _WorldContextObject ? _WorldContextObject->GetWorld() : nullptr;
    if (!World) return false;
    const auto Params = QueryParams(_WorldContextObject, _bTraceComplex, _ActorsToIgnore, _bIgnoreSelf);
    const auto Shape = FCollisionShape::MakeCapsule(_Radius, _HalfHeight);
    const auto Rotation = FQuat::Identity;
    const bool Result = World->SweepSingleByChannel(_OutHit, _Start, _End, Rotation, UEngineTypes::ConvertToCollisionChannel(_TraceChannel), Shape, Params);
    DrawTrace(World, _Start, _End, &Shape, Rotation, _DrawDebugType, Result ? std::vector<FHitResult>{_OutHit} : std::vector<FHitResult>{}, _TraceColor, _TraceHitColor, _DrawTime);
    return Result;
}
bool UKismetSystemLibrary::CapsuleTraceMulti(const UObject* _WorldContextObject, const FVector _Start, const FVector _End, float _Radius, float _HalfHeight, ETraceTypeQuery _TraceChannel, bool _bTraceComplex, const std::vector<AActor*>& _ActorsToIgnore, EDrawDebugTrace::Type _DrawDebugType, std::vector<FHitResult>& _OutHits, bool _bIgnoreSelf, FLinearColor _TraceColor, FLinearColor _TraceHitColor, float _DrawTime)
{
    _OutHits.clear();
    auto* World = _WorldContextObject ? _WorldContextObject->GetWorld() : nullptr;
    if (!World) return false;
    const auto Params = QueryParams(_WorldContextObject, _bTraceComplex, _ActorsToIgnore, _bIgnoreSelf);
    const auto Shape = FCollisionShape::MakeCapsule(_Radius, _HalfHeight);
    const auto Rotation = FQuat::Identity;
    const bool Result = World->SweepMultiByChannel(_OutHits, _Start, _End, Rotation, UEngineTypes::ConvertToCollisionChannel(_TraceChannel), Shape, Params);
    DrawTrace(World, _Start, _End, &Shape, Rotation, _DrawDebugType, _OutHits, _TraceColor, _TraceHitColor, _DrawTime);
    return Result;
}
bool UKismetSystemLibrary::CapsuleTraceSingleForObjects(const UObject* _WorldContextObject, const FVector _Start, const FVector _End, float _Radius, float _HalfHeight, const std::vector<EObjectTypeQuery>& _ObjectTypes, bool _bTraceComplex, const std::vector<AActor*>& _ActorsToIgnore, EDrawDebugTrace::Type _DrawDebugType, FHitResult& _OutHit, bool _bIgnoreSelf, FLinearColor _TraceColor, FLinearColor _TraceHitColor, float _DrawTime)
{
    _OutHit = {}; _OutHit.TraceStart = _Start; _OutHit.TraceEnd = _End;
    auto* World = _WorldContextObject ? _WorldContextObject->GetWorld() : nullptr;
    if (!World) return false;
    const auto Params = QueryParams(_WorldContextObject, _bTraceComplex, _ActorsToIgnore, _bIgnoreSelf);
    const auto Shape = FCollisionShape::MakeCapsule(_Radius, _HalfHeight);
    const auto Rotation = FQuat::Identity;
    const bool Result = World->SweepSingleByObjectType(_OutHit, _Start, _End, Rotation, ObjectParams(_ObjectTypes), Shape, Params);
    DrawTrace(World, _Start, _End, &Shape, Rotation, _DrawDebugType, Result ? std::vector<FHitResult>{_OutHit} : std::vector<FHitResult>{}, _TraceColor, _TraceHitColor, _DrawTime);
    return Result;
}
bool UKismetSystemLibrary::CapsuleTraceMultiForObjects(const UObject* _WorldContextObject, const FVector _Start, const FVector _End, float _Radius, float _HalfHeight, const std::vector<EObjectTypeQuery>& _ObjectTypes, bool _bTraceComplex, const std::vector<AActor*>& _ActorsToIgnore, EDrawDebugTrace::Type _DrawDebugType, std::vector<FHitResult>& _OutHits, bool _bIgnoreSelf, FLinearColor _TraceColor, FLinearColor _TraceHitColor, float _DrawTime)
{
    _OutHits.clear();
    auto* World = _WorldContextObject ? _WorldContextObject->GetWorld() : nullptr;
    if (!World) return false;
    const auto Params = QueryParams(_WorldContextObject, _bTraceComplex, _ActorsToIgnore, _bIgnoreSelf);
    const auto Shape = FCollisionShape::MakeCapsule(_Radius, _HalfHeight);
    const auto Rotation = FQuat::Identity;
    const bool Result = World->SweepMultiByObjectType(_OutHits, _Start, _End, Rotation, ObjectParams(_ObjectTypes), Shape, Params);
    DrawTrace(World, _Start, _End, &Shape, Rotation, _DrawDebugType, _OutHits, _TraceColor, _TraceHitColor, _DrawTime);
    return Result;
}
bool UKismetSystemLibrary::CapsuleTraceSingleByProfile(const UObject* _WorldContextObject, const FVector _Start, const FVector _End, float _Radius, float _HalfHeight, FName _ProfileName, bool _bTraceComplex, const std::vector<AActor*>& _ActorsToIgnore, EDrawDebugTrace::Type _DrawDebugType, FHitResult& _OutHit, bool _bIgnoreSelf, FLinearColor _TraceColor, FLinearColor _TraceHitColor, float _DrawTime)
{
    _OutHit = {}; _OutHit.TraceStart = _Start; _OutHit.TraceEnd = _End;
    auto* World = _WorldContextObject ? _WorldContextObject->GetWorld() : nullptr;
    if (!World) return false;
    const auto Params = QueryParams(_WorldContextObject, _bTraceComplex, _ActorsToIgnore, _bIgnoreSelf);
    const auto Shape = FCollisionShape::MakeCapsule(_Radius, _HalfHeight);
    const auto Rotation = FQuat::Identity;
    const bool Result = World->SweepSingleByProfile(_OutHit, _Start, _End, Rotation, _ProfileName, Shape, Params);
    DrawTrace(World, _Start, _End, &Shape, Rotation, _DrawDebugType, Result ? std::vector<FHitResult>{_OutHit} : std::vector<FHitResult>{}, _TraceColor, _TraceHitColor, _DrawTime);
    return Result;
}
bool UKismetSystemLibrary::CapsuleTraceMultiByProfile(const UObject* _WorldContextObject, const FVector _Start, const FVector _End, float _Radius, float _HalfHeight, FName _ProfileName, bool _bTraceComplex, const std::vector<AActor*>& _ActorsToIgnore, EDrawDebugTrace::Type _DrawDebugType, std::vector<FHitResult>& _OutHits, bool _bIgnoreSelf, FLinearColor _TraceColor, FLinearColor _TraceHitColor, float _DrawTime)
{
    _OutHits.clear();
    auto* World = _WorldContextObject ? _WorldContextObject->GetWorld() : nullptr;
    if (!World) return false;
    const auto Params = QueryParams(_WorldContextObject, _bTraceComplex, _ActorsToIgnore, _bIgnoreSelf);
    const auto Shape = FCollisionShape::MakeCapsule(_Radius, _HalfHeight);
    const auto Rotation = FQuat::Identity;
    const bool Result = World->SweepMultiByProfile(_OutHits, _Start, _End, Rotation, _ProfileName, Shape, Params);
    DrawTrace(World, _Start, _End, &Shape, Rotation, _DrawDebugType, _OutHits, _TraceColor, _TraceHitColor, _DrawTime);
    return Result;
}
bool UKismetSystemLibrary::BoxOverlapActors(const UObject* _WorldContextObject, const FVector _Pos, FVector _BoxExtent, const std::vector<EObjectTypeQuery>& _ObjectTypes, UClass* _ClassFilter, const std::vector<AActor*>& _ActorsToIgnore, std::vector<AActor*>& _OutActors)
{
    _OutActors.clear();
    auto* World = _WorldContextObject ? _WorldContextObject->GetWorld() : nullptr;
    if (!World) return false;
    const auto Params = QueryParams(_WorldContextObject, false, _ActorsToIgnore, false);
    std::vector<FOverlapResult> Overlaps;
    World->OverlapMultiByObjectType(Overlaps, _Pos, FQuat::Identity, ObjectParams(_ObjectTypes), FCollisionShape::MakeBox(_BoxExtent), Params);
    for (const auto& Overlap : Overlaps)
    {
        auto* Object = Overlap.GetActor();
        if (Object && (!_ClassFilter || Object->IsA(_ClassFilter)) && std::find(_OutActors.begin(), _OutActors.end(), Object) == _OutActors.end()) _OutActors.push_back(Object);
    }
    return !_OutActors.empty();
}
bool UKismetSystemLibrary::BoxOverlapComponents(const UObject* _WorldContextObject, const FVector _Pos, FVector _BoxExtent, const std::vector<EObjectTypeQuery>& _ObjectTypes, UClass* _ClassFilter, const std::vector<AActor*>& _ActorsToIgnore, std::vector<UPrimitiveComponent*>& _OutComponents)
{
    _OutComponents.clear();
    auto* World = _WorldContextObject ? _WorldContextObject->GetWorld() : nullptr;
    if (!World) return false;
    const auto Params = QueryParams(_WorldContextObject, false, _ActorsToIgnore, false);
    std::vector<FOverlapResult> Overlaps;
    World->OverlapMultiByObjectType(Overlaps, _Pos, FQuat::Identity, ObjectParams(_ObjectTypes), FCollisionShape::MakeBox(_BoxExtent), Params);
    for (const auto& Overlap : Overlaps)
    {
        auto* Object = Overlap.GetComponent();
        if (Object && (!_ClassFilter || Object->IsA(_ClassFilter)) && std::find(_OutComponents.begin(), _OutComponents.end(), Object) == _OutComponents.end()) _OutComponents.push_back(Object);
    }
    return !_OutComponents.empty();
}
bool UKismetSystemLibrary::SphereOverlapActors(const UObject* _WorldContextObject, const FVector _Pos, float _Radius, const std::vector<EObjectTypeQuery>& _ObjectTypes, UClass* _ClassFilter, const std::vector<AActor*>& _ActorsToIgnore, std::vector<AActor*>& _OutActors)
{
    _OutActors.clear();
    auto* World = _WorldContextObject ? _WorldContextObject->GetWorld() : nullptr;
    if (!World) return false;
    const auto Params = QueryParams(_WorldContextObject, false, _ActorsToIgnore, false);
    std::vector<FOverlapResult> Overlaps;
    World->OverlapMultiByObjectType(Overlaps, _Pos, FQuat::Identity, ObjectParams(_ObjectTypes), FCollisionShape::MakeSphere(_Radius), Params);
    for (const auto& Overlap : Overlaps)
    {
        auto* Object = Overlap.GetActor();
        if (Object && (!_ClassFilter || Object->IsA(_ClassFilter)) && std::find(_OutActors.begin(), _OutActors.end(), Object) == _OutActors.end()) _OutActors.push_back(Object);
    }
    return !_OutActors.empty();
}
bool UKismetSystemLibrary::SphereOverlapComponents(const UObject* _WorldContextObject, const FVector _Pos, float _Radius, const std::vector<EObjectTypeQuery>& _ObjectTypes, UClass* _ClassFilter, const std::vector<AActor*>& _ActorsToIgnore, std::vector<UPrimitiveComponent*>& _OutComponents)
{
    _OutComponents.clear();
    auto* World = _WorldContextObject ? _WorldContextObject->GetWorld() : nullptr;
    if (!World) return false;
    const auto Params = QueryParams(_WorldContextObject, false, _ActorsToIgnore, false);
    std::vector<FOverlapResult> Overlaps;
    World->OverlapMultiByObjectType(Overlaps, _Pos, FQuat::Identity, ObjectParams(_ObjectTypes), FCollisionShape::MakeSphere(_Radius), Params);
    for (const auto& Overlap : Overlaps)
    {
        auto* Object = Overlap.GetComponent();
        if (Object && (!_ClassFilter || Object->IsA(_ClassFilter)) && std::find(_OutComponents.begin(), _OutComponents.end(), Object) == _OutComponents.end()) _OutComponents.push_back(Object);
    }
    return !_OutComponents.empty();
}
bool UKismetSystemLibrary::CapsuleOverlapActors(const UObject* _WorldContextObject, const FVector _Pos, float _Radius, float _HalfHeight, const std::vector<EObjectTypeQuery>& _ObjectTypes, UClass* _ClassFilter, const std::vector<AActor*>& _ActorsToIgnore, std::vector<AActor*>& _OutActors)
{
    _OutActors.clear();
    auto* World = _WorldContextObject ? _WorldContextObject->GetWorld() : nullptr;
    if (!World) return false;
    const auto Params = QueryParams(_WorldContextObject, false, _ActorsToIgnore, false);
    std::vector<FOverlapResult> Overlaps;
    World->OverlapMultiByObjectType(Overlaps, _Pos, FQuat::Identity, ObjectParams(_ObjectTypes), FCollisionShape::MakeCapsule(_Radius, _HalfHeight), Params);
    for (const auto& Overlap : Overlaps)
    {
        auto* Object = Overlap.GetActor();
        if (Object && (!_ClassFilter || Object->IsA(_ClassFilter)) && std::find(_OutActors.begin(), _OutActors.end(), Object) == _OutActors.end()) _OutActors.push_back(Object);
    }
    return !_OutActors.empty();
}
bool UKismetSystemLibrary::CapsuleOverlapComponents(const UObject* _WorldContextObject, const FVector _Pos, float _Radius, float _HalfHeight, const std::vector<EObjectTypeQuery>& _ObjectTypes, UClass* _ClassFilter, const std::vector<AActor*>& _ActorsToIgnore, std::vector<UPrimitiveComponent*>& _OutComponents)
{
    _OutComponents.clear();
    auto* World = _WorldContextObject ? _WorldContextObject->GetWorld() : nullptr;
    if (!World) return false;
    const auto Params = QueryParams(_WorldContextObject, false, _ActorsToIgnore, false);
    std::vector<FOverlapResult> Overlaps;
    World->OverlapMultiByObjectType(Overlaps, _Pos, FQuat::Identity, ObjectParams(_ObjectTypes), FCollisionShape::MakeCapsule(_Radius, _HalfHeight), Params);
    for (const auto& Overlap : Overlaps)
    {
        auto* Object = Overlap.GetComponent();
        if (Object && (!_ClassFilter || Object->IsA(_ClassFilter)) && std::find(_OutComponents.begin(), _OutComponents.end(), Object) == _OutComponents.end()) _OutComponents.push_back(Object);
    }
    return !_OutComponents.empty();
}
