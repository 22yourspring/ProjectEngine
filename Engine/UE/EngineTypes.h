#pragma once
#include "CoreTypes.h"
#include <array>

enum ECollisionChannel : int
{
    ECC_WorldStatic, ECC_WorldDynamic, ECC_Pawn, ECC_Visibility, ECC_Camera,
    ECC_PhysicsBody, ECC_Vehicle, ECC_Destructible,
    ECC_EngineTraceChannel1, ECC_EngineTraceChannel2, ECC_EngineTraceChannel3,
    ECC_EngineTraceChannel4, ECC_EngineTraceChannel5, ECC_EngineTraceChannel6,
    ECC_GameTraceChannel1, ECC_GameTraceChannel2, ECC_GameTraceChannel3,
    ECC_GameTraceChannel4, ECC_GameTraceChannel5, ECC_GameTraceChannel6,
    ECC_GameTraceChannel7, ECC_GameTraceChannel8, ECC_GameTraceChannel9,
    ECC_GameTraceChannel10, ECC_GameTraceChannel11, ECC_GameTraceChannel12,
    ECC_GameTraceChannel13, ECC_GameTraceChannel14, ECC_GameTraceChannel15,
    ECC_GameTraceChannel16, ECC_GameTraceChannel17, ECC_GameTraceChannel18,
    ECC_OverlapAll_Deprecated, ECC_MAX
};
enum ETraceTypeQuery : int
{
    TraceTypeQuery1, TraceTypeQuery2, TraceTypeQuery3, TraceTypeQuery4, TraceTypeQuery5, TraceTypeQuery6, TraceTypeQuery7, TraceTypeQuery8, TraceTypeQuery9, TraceTypeQuery10, TraceTypeQuery11, TraceTypeQuery12, TraceTypeQuery13, TraceTypeQuery14, TraceTypeQuery15, TraceTypeQuery16, TraceTypeQuery17, TraceTypeQuery18, TraceTypeQuery19, TraceTypeQuery20, TraceTypeQuery21, TraceTypeQuery22, TraceTypeQuery23, TraceTypeQuery24, TraceTypeQuery25, TraceTypeQuery26, TraceTypeQuery27, TraceTypeQuery28, TraceTypeQuery29, TraceTypeQuery30, TraceTypeQuery31, TraceTypeQuery32,
    ETraceTypeQuery_MAX
};
enum EObjectTypeQuery : int
{
    ObjectTypeQuery1, ObjectTypeQuery2, ObjectTypeQuery3, ObjectTypeQuery4, ObjectTypeQuery5, ObjectTypeQuery6, ObjectTypeQuery7, ObjectTypeQuery8, ObjectTypeQuery9, ObjectTypeQuery10, ObjectTypeQuery11, ObjectTypeQuery12, ObjectTypeQuery13, ObjectTypeQuery14, ObjectTypeQuery15, ObjectTypeQuery16, ObjectTypeQuery17, ObjectTypeQuery18, ObjectTypeQuery19, ObjectTypeQuery20, ObjectTypeQuery21, ObjectTypeQuery22, ObjectTypeQuery23, ObjectTypeQuery24, ObjectTypeQuery25, ObjectTypeQuery26, ObjectTypeQuery27, ObjectTypeQuery28, ObjectTypeQuery29, ObjectTypeQuery30, ObjectTypeQuery31, ObjectTypeQuery32,
    EObjectTypeQuery_MAX
};
class ENGINE_API UEngineTypes
{
public:
    static ECollisionChannel ConvertToCollisionChannel(ETraceTypeQuery _TraceType);
    static ECollisionChannel ConvertToCollisionChannel(EObjectTypeQuery _ObjectType);
    static ETraceTypeQuery ConvertToTraceType(ECollisionChannel _Channel);
    static EObjectTypeQuery ConvertToObjectType(ECollisionChannel _Channel);
};
enum ECollisionResponse : int { ECR_Ignore, ECR_Overlap, ECR_Block, ECR_MAX };
namespace ECollisionEnabled
{
    enum Type : int { NoCollision, QueryOnly, PhysicsOnly, QueryAndPhysics, ProbeOnly, QueryAndProbe };
}
namespace EDOFMode
{
    enum Type : int { Default, SixDOF, YZPlane, XZPlane, XYPlane, CustomPlane, None };
}
struct FCollisionResponseContainer
{
    explicit FCollisionResponseContainer(ECollisionResponse _DefaultResponse = ECR_Block) { SetAllChannels(_DefaultResponse); }
    bool SetResponse(ECollisionChannel _Channel, ECollisionResponse _Response)
    {
        if (_Channel < 0 || _Channel >= 32 || _Response < 0 || _Response >= ECR_MAX) return false;
        __Responses[_Channel] = _Response; return true;
    }
    bool SetAllChannels(ECollisionResponse _Response)
    {
        if (_Response < ECR_Ignore || _Response >= ECR_MAX) return false;
        __Responses.fill(_Response); return true;
    }
    ECollisionResponse GetResponse(ECollisionChannel _Channel) const { return _Channel >= 0 && _Channel < 32 ? __Responses[_Channel] : ECR_Ignore; }
private:
    std::array<ECollisionResponse, 32> __Responses{};
};
inline bool CollisionHasQuery(ECollisionEnabled::Type _Type)
{
    return _Type == ECollisionEnabled::QueryOnly || _Type == ECollisionEnabled::QueryAndPhysics || _Type == ECollisionEnabled::QueryAndProbe;
}
inline bool CollisionHasPhysics(ECollisionEnabled::Type _Type)
{
    return _Type == ECollisionEnabled::PhysicsOnly || _Type == ECollisionEnabled::QueryAndPhysics || _Type == ECollisionEnabled::ProbeOnly || _Type == ECollisionEnabled::QueryAndProbe;
}
