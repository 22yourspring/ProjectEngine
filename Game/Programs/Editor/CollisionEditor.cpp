#include "EditorImGui.h"
#include "UE/CollisionProfile.h"
#include "UE/BoxComponent.h"
#include "UE/SphereComponent.h"
#include "UE/CapsuleComponent.h"
#include "UE/GameEngine.h"
#include "UE/World.h"
#include "UE/PathEngineSystem.h"
#include <algorithm>
#include <cmath>
#include <cctype>

namespace
{
    const char* EnabledNames[] = {"No Collision", "Query Only (No Physics Collision)", "Physics Only (No Query Collision)", "Collision Enabled (Query and Physics)", "Probe Only (No Query or Physics Collision)", "Query and Probe (No Physics Collision)"};
    bool DrawObjectType(ECollisionChannel& _Object, const char* _Label = "Object Type")
    {
        bool Changed = false; std::string Preview;
        for (const auto& Channel : UCollisionProfile::Get()->GetChannels()) if (Channel.Channel == _Object) Preview = Channel.Name.ToString().ToUtf8();
        if (ImGui::BeginCombo(_Label, Preview.c_str()))
        {
            for (const auto& Channel : UCollisionProfile::Get()->GetChannels())
                if (!Channel.bTraceType && ImGui::Selectable(Channel.Name.ToString().ToUtf8().c_str(), _Object == Channel.Channel)) { _Object = Channel.Channel; Changed = true; }
            ImGui::EndCombo();
        }
        return Changed;
    }
    bool DrawResponses(FCollisionResponseContainer& _Responses)
    {
        bool Changed = false;
        if (ImGui::BeginTable("CollisionResponses", 4, ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_SizingStretchProp))
        {
            ImGui::TableSetupColumn("Collision Responses", ImGuiTableColumnFlags_WidthStretch, 3);
            ImGui::TableSetupColumn("Ignore"); ImGui::TableSetupColumn("Overlap"); ImGui::TableSetupColumn("Block"); ImGui::TableHeadersRow();
            ImGui::TableNextRow(); ImGui::TableNextColumn(); ImGui::TextUnformatted("All Channels");
            for (int Response = 0; Response < 3; ++Response)
            {
                ImGui::TableNextColumn(); ImGui::PushID(Response);
                bool All = true;
                for (const auto& Channel : UCollisionProfile::Get()->GetChannels()) All &= _Responses.GetResponse(Channel.Channel) == Response;
                if (ImGui::Checkbox("##All", &All)) { _Responses.SetAllChannels(static_cast<ECollisionResponse>(Response)); Changed = true; }
                ImGui::PopID();
            }
            for (bool Trace : {true, false})
            {
                ImGui::TableNextRow(); ImGui::TableNextColumn(); ImGui::TextDisabled(Trace ? "Trace Responses" : "Object Responses");
                for (const auto& Channel : UCollisionProfile::Get()->GetChannels())
                {
                    if (Channel.bTraceType != Trace) continue;
                    ImGui::PushID(int(Channel.Channel)); ImGui::TableNextRow(); ImGui::TableNextColumn(); ImGui::TextUnformatted(Channel.Name.ToString().ToUtf8().c_str());
                    for (int Response = 0; Response < 3; ++Response)
                    {
                        ImGui::TableNextColumn(); ImGui::PushID(Response);
                        bool Selected = _Responses.GetResponse(Channel.Channel) == Response;
                        if (ImGui::Checkbox("##Response", &Selected)) { _Responses.SetResponse(Channel.Channel, static_cast<ECollisionResponse>(Response)); Changed = true; }
                        ImGui::PopID();
                    }
                    ImGui::PopID();
                }
            }
            ImGui::EndTable();
        }
        return Changed;
    }
}
void FEditorImGui::DrawCollisionDetails(UPrimitiveComponent* _Component)
{
    auto* Owner = _Component->GetOwner();
    if (Owner && Owner->GetRootComponent() != _Component && ImGui::Button("Make Root"))
    {
        auto* Previous = Owner->GetRootComponent();
        const auto OldTransform = Previous ? Previous->GetComponentTransform() : FTransform();
        _Component->DetachFromComponent(); Owner->SetRootComponent(_Component);
        if (Previous)
        {
            Previous->SetupAttachment(_Component); Previous->SetWorldLocation(OldTransform.GetLocation()); Previous->SetWorldRotation(OldTransform.GetRotation());
            const auto Scale = _Component->GetComponentScale();
            if (std::abs(Scale.X) > 1.e-6 && std::abs(Scale.Y) > 1.e-6 && std::abs(Scale.Z) > 1.e-6) Previous->SetRelativeScale3D(OldTransform.GetScale3D() / Scale);
        }
    }
    auto& Body = _Component->BodyInstance;
    auto Category = [&](const char* _Name, const char* _Keywords)
    {
        std::string Search = __DetailsSearch;
        std::string Terms = std::string(_Name) + " " + _Keywords;
        std::transform(Search.begin(), Search.end(), Search.begin(), [](unsigned char _C) { return char(std::tolower(_C)); });
        std::transform(Terms.begin(), Terms.end(), Terms.begin(), [](unsigned char _C) { return char(std::tolower(_C)); });
        return (Search.empty() || Terms.find(Search) != std::string::npos) && ImGui::CollapsingHeader(_Name, ImGuiTreeNodeFlags_DefaultOpen);
    };
    auto BeginRows = [](const char* _Id)
    {
        if (!ImGui::BeginTable(_Id, 2, ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_RowBg)) return false;
        ImGui::TableSetupColumn("Property", ImGuiTableColumnFlags_WidthStretch, 0.48f);
        ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch, 0.52f);
        return true;
    };
    auto Row = [](const char* _Label)
    {
        ImGui::TableNextRow(); ImGui::TableNextColumn(); ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(_Label); ImGui::TableNextColumn(); ImGui::SetNextItemWidth(-1);
    };
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 2.f);
    ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(8, 3));
    ImGui::PushStyleColor(ImGuiCol_TableRowBg, ImVec4(.115f, .115f, .115f, 1));
    ImGui::PushStyleColor(ImGuiCol_TableRowBgAlt, ImVec4(.125f, .125f, .125f, 1));
    if (Category("Physics", "Simulate Physics Linear Damping Angular Damping Enable Gravity Constraints Mode Lock Position Rotation CCD"))
    {
        if (BeginRows("PhysicsProperties"))
        {
            bool Simulate = _Component->IsSimulatingPhysics(), Gravity = _Component->IsGravityEnabled();
            Row("Simulate Physics");
            const bool HasShape = !_Component->GetCollisionShape().IsNearlyZero();
            ImGui::BeginDisabled(!HasShape);
            if (ImGui::Checkbox("##SimulatePhysics", &Simulate)) _Component->SetSimulatePhysics(Simulate);
            ImGui::EndDisabled();
            if (!HasShape && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
                ImGui::SetTooltip("Add a Box, Sphere or Capsule Collision component and select it to simulate physics.");
            Row("Linear Damping"); float Linear = _Component->GetLinearDamping();
            if (ImGui::DragFloat("##LinearDamping", &Linear, .01f, 0, 1.e6f, "%.3f", ImGuiSliderFlags_AlwaysClamp)) _Component->SetLinearDamping(Linear);
            Row("Angular Damping"); float Angular = _Component->GetAngularDamping();
            if (ImGui::DragFloat("##AngularDamping", &Angular, .01f, 0, 1.e6f, "%.3f", ImGuiSliderFlags_AlwaysClamp)) _Component->SetAngularDamping(Angular);
            Row("Enable Gravity");
            if (ImGui::Checkbox("##EnableGravity", &Gravity)) _Component->SetEnableGravity(Gravity);
            ImGui::EndTable();
        }
        if (ImGui::TreeNodeEx("Constraints", ImGuiTreeNodeFlags_DefaultOpen))
        {
            if (BeginRows("ConstraintsProperties"))
            {
                const char* Modes[] = {"Default", "Six DOF", "YZ Plane", "XZ Plane", "XY Plane", "Custom Plane", "None"};
                int Mode = Body.GetDOFLock();
                Row("Mode");
                if (ImGui::Combo("##DOFMode", &Mode, Modes, 7)) Body.SetDOFLock(static_cast<EDOFMode::Type>(Mode));
                if (Mode == EDOFMode::SixDOF)
                {
                    auto Axes = [&](const char* _Name, bool& _X, bool& _Y, bool& _Z)
                    {
                        Row(_Name); ImGui::PushID(_Name);
                        bool Changed = ImGui::Checkbox("X", &_X); ImGui::SameLine();
                        Changed |= ImGui::Checkbox("Y", &_Y); ImGui::SameLine();
                        Changed |= ImGui::Checkbox("Z", &_Z); ImGui::PopID();
                        if (Changed) Body.SetDOFLock(EDOFMode::SixDOF);
                    };
                    Axes("Lock Position", Body.bLockXTranslation, Body.bLockYTranslation, Body.bLockZTranslation);
                    Axes("Lock Rotation", Body.bLockXRotation, Body.bLockYRotation, Body.bLockZRotation);
                }
                if (Mode == EDOFMode::CustomPlane)
                {
                    Row("Custom Plane Normal");
                    float Normal[] = {float(Body.CustomDOFPlaneNormal.X), float(Body.CustomDOFPlaneNormal.Y), float(Body.CustomDOFPlaneNormal.Z)};
                    if (ImGui::DragFloat3("##PlaneNormal", Normal, .01f))
                    { Body.CustomDOFPlaneNormal = FVector(Normal[0], Normal[1], Normal[2]); Body.SetDOFLock(EDOFMode::CustomPlane); }
                }
                ImGui::EndTable();
            }
            ImGui::TreePop();
        }
    }
    if (Category("Collision", "Simulation Generates Hit Events Generate Overlap Events Collision Presets Enabled Object Type Responses Ignore Overlap Block Use CCD"))
    {
        if (BeginRows("CollisionProperties"))
        {
            bool Notify = Body.bNotifyRigidBodyCollision, Overlaps = _Component->GetGenerateOverlapEvents(), CCD = Body.bUseCCD;
            Row("Simulation Generates Hit Events");
            if (ImGui::Checkbox("##HitEvents", &Notify)) _Component->SetNotifyRigidBodyCollision(Notify);
            Row("Generate Overlap Events");
            if (ImGui::Checkbox("##OverlapEvents", &Overlaps)) _Component->SetGenerateOverlapEvents(Overlaps);
            Row("Collision Presets");
            const auto ProfileName = _Component->GetCollisionProfileName();
            const auto Preview = ProfileName == FName(TEXT("Custom")) ? std::string("Custom...") : ProfileName.ToString().ToUtf8();
            if (ImGui::BeginCombo("##CollisionPresets", Preview.c_str()))
            {
                if (ImGui::Selectable("Custom...", ProfileName == FName(TEXT("Custom")))) _Component->SetCollisionProfileName(FName(TEXT("Custom")));
                for (const auto& Profile : UCollisionProfile::Get()->GetProfiles())
                    if (ImGui::Selectable(Profile.Name.ToString().ToUtf8().c_str(), Profile.Name == ProfileName)) _Component->SetCollisionProfileName(Profile.Name);
                ImGui::EndCombo();
            }
            ImGui::BeginDisabled(_Component->GetCollisionProfileName() != FName(TEXT("Custom")));
            Row("Collision Enabled"); int Enabled = _Component->GetCollisionEnabled();
            if (ImGui::Combo("##CollisionEnabled", &Enabled, EnabledNames, 6)) _Component->SetCollisionEnabled(static_cast<ECollisionEnabled::Type>(Enabled));
            Row("Object Type"); auto Object = _Component->GetCollisionObjectType();
            if (DrawObjectType(Object, "##ObjectType")) _Component->SetCollisionObjectType(Object);
            ImGui::EndDisabled();
            Row("Use CCD");
            if (ImGui::Checkbox("##CCD", &CCD)) { Body.bUseCCD = CCD; Body.UpdatePhysicsFilterData(); }
            ImGui::EndTable();
        }
        ImGui::BeginDisabled(_Component->GetCollisionProfileName() != FName(TEXT("Custom")));
        auto Responses = Body.GetResponseToChannels();
        if (DrawResponses(Responses)) _Component->SetCollisionResponseToChannels(Responses);
        ImGui::EndDisabled();
    }
    if (Category("Shape", "Box Extent Sphere Radius Capsule Radius Half Height") && BeginRows("ShapeProperties"))
    {
        if (auto* Box = dynamic_cast<UBoxComponent*>(_Component))
        {
            Row("Box Extent"); const auto Extent = Box->GetUnscaledBoxExtent();
            float Values[] = {float(Extent.X), float(Extent.Y), float(Extent.Z)};
            if (ImGui::DragFloat3("##BoxExtent", Values, .5f, .01f, 100000.f, "%.2f", ImGuiSliderFlags_AlwaysClamp))
                Box->SetBoxExtent(FVector(Values[0], Values[1], Values[2]));
        }
        else if (auto* Sphere = dynamic_cast<USphereComponent*>(_Component))
        {
            Row("Sphere Radius"); float Radius = Sphere->GetUnscaledSphereRadius();
            if (ImGui::DragFloat("##SphereRadius", &Radius, .5f, .01f, 100000.f, "%.2f", ImGuiSliderFlags_AlwaysClamp)) Sphere->SetSphereRadius(Radius);
        }
        else if (auto* Capsule = dynamic_cast<UCapsuleComponent*>(_Component))
        {
            float Radius = Capsule->GetUnscaledCapsuleRadius(), Height = Capsule->GetUnscaledCapsuleHalfHeight();
            Row("Capsule Radius"); bool Changed = ImGui::DragFloat("##CapsuleRadius", &Radius, .5f, .01f, 100000.f, "%.2f", ImGuiSliderFlags_AlwaysClamp);
            Row("Capsule Half Height"); Changed |= ImGui::DragFloat("##CapsuleHeight", &Height, .5f, .01f, 100000.f, "%.2f", ImGuiSliderFlags_AlwaysClamp);
            if (Changed) Capsule->SetCapsuleSize(Radius, Height);
        }
        else { Row("Collision Geometry"); ImGui::TextWrapped("Add Box, Sphere or Capsule Collision."); }
        ImGui::EndTable();
    }
    ImGui::PopStyleColor(2); ImGui::PopStyleVar(2);
}
void FEditorImGui::DrawCollisionSettings()
{
    if (ImGui::Button("Open Collision Test")) __ShowCollisionPreview = true;
    auto* Engine = dynamic_cast<UGameEngine*>(GEngine); if (!Engine) return;
    auto* Paths = Engine->GetEngineSystem<PathEngineSystem>();
    Engine->WithWorld([&](UWorld* _World)
    {
        auto* Profiles = UCollisionProfile::Get(); bool Changed = false;
        static char Name[129] = {}; static char Description[4097] = {}; static int Response = ECR_Block; static bool Trace = false; static int EditingChannel = -1;
        static FCollisionResponseTemplate Editing; static bool NewProfile = false;
        for (bool IsTrace : {false, true})
        {
            if (!ImGui::CollapsingHeader(IsTrace ? "Trace Channels" : "Object Channels", ImGuiTreeNodeFlags_DefaultOpen)) continue;
            ImGui::PushID(IsTrace ? 1 : 0);
            if (ImGui::Button(IsTrace ? "New Trace Channel..." : "New Object Channel...")) { Trace = IsTrace; Name[0] = 0; Response = ECR_Block; EditingChannel = -1; ImGui::OpenPopup("Collision Channel"); }
            if (ImGui::BeginPopupModal("Collision Channel", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
            {
                ImGui::InputText("Name", Name, sizeof(Name)); ImGui::Combo("Default Response", &Response, "Ignore\0Overlap\0Block\0");
                if (ImGui::Button("Accept"))
                {
                    const bool Saved = EditingChannel < 0 ? Profiles->AddChannel(FName(Name), static_cast<ECollisionResponse>(Response), Trace) : Profiles->EditChannel(static_cast<ECollisionChannel>(EditingChannel), FName(Name), static_cast<ECollisionResponse>(Response));
                    if (Saved) { Changed = true; ImGui::CloseCurrentPopup(); __CollisionSettingsError = {}; }
                    else __CollisionSettingsError = TEXT("Use a unique name. At most 18 custom channels are supported.");
                }
                ImGui::SameLine(); if (ImGui::Button("Cancel")) ImGui::CloseCurrentPopup();
                if (!__CollisionSettingsError.IsEmpty()) ImGui::TextWrapped("%s", __CollisionSettingsError.ToUtf8().c_str());
                ImGui::EndPopup();
            }
            const auto Channels = Profiles->GetChannels();
            bool OpenChannelEditor = false;
            if (ImGui::BeginTable("Channels", 3, ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_SizingStretchProp))
            {
                ImGui::TableSetupColumn("Name"); ImGui::TableSetupColumn("Default Response"); ImGui::TableSetupColumn("Actions"); ImGui::TableHeadersRow();
                for (const auto& Channel : Channels)
                {
                    if (Channel.bTraceType != IsTrace || Channel.Channel < ECC_GameTraceChannel1) continue;
                    ImGui::PushID(int(Channel.Channel)); ImGui::TableNextRow(); ImGui::TableNextColumn();
                    ImGui::TextUnformatted(Channel.Name.ToString().ToUtf8().c_str()); ImGui::TableNextColumn();
                    ImGui::TextUnformatted(Channel.DefaultResponse == ECR_Block ? "Block" : Channel.DefaultResponse == ECR_Overlap ? "Overlap" : "Ignore"); ImGui::TableNextColumn();
                    if (ImGui::SmallButton("Edit..."))
                    {
                        EditingChannel = Channel.Channel; Trace = IsTrace; Response = Channel.DefaultResponse; strcpy_s(Name, Channel.Name.ToString().ToUtf8().c_str());
                        OpenChannelEditor = true;
                    }
                    ImGui::SameLine(); if (ImGui::SmallButton("Delete"))
                    {
                        Changed |= Profiles->RemoveChannel(Channel.Channel);
                        if (_World) for (auto* Level : _World->GetLevels()) for (const auto& Actor : Level->GetActors()) for (auto* Component : Actor->GetComponents())
                            if (auto* Primitive = dynamic_cast<UPrimitiveComponent*>(Component); Primitive && Primitive->GetCollisionProfileName() == FName(TEXT("Custom")))
                            {
                                if (Primitive->GetCollisionObjectType() == Channel.Channel) Primitive->SetCollisionObjectType(ECC_WorldStatic);
                                Primitive->SetCollisionResponseToChannel(Channel.Channel, ECR_Block);
                            }
                    }
                    ImGui::PopID();
                }
                ImGui::EndTable();
            }
            if (OpenChannelEditor) ImGui::OpenPopup("Collision Channel");
            ImGui::PopID();
        }
        if (ImGui::CollapsingHeader("Preset", ImGuiTreeNodeFlags_DefaultOpen))
        {
            if (ImGui::Button("New...")) { Editing = FCollisionResponseTemplate(); Name[0] = Description[0] = 0; NewProfile = true; ImGui::OpenPopup("Collision Preset"); }
            const auto Templates = Profiles->GetProfiles();
            for (const auto& Profile : Templates)
            {
                ImGui::PushID(Profile.Name.ToString().ToUtf8().c_str());
                ImGui::TextUnformatted(Profile.Name.ToString().ToUtf8().c_str()); ImGui::SameLine();
                if (ImGui::SmallButton("Edit..."))
                {
                    Editing = Profile; NewProfile = false; strcpy_s(Name, Profile.Name.ToString().ToUtf8().c_str());
                    strncpy_s(Description, Profile.HelpMessage.ToUtf8().c_str(), _TRUNCATE);
                    ImGui::PopID(); ImGui::OpenPopup("Collision Preset"); ImGui::PushID(Profile.Name.ToString().ToUtf8().c_str());
                }
                ImGui::SameLine(); ImGui::BeginDisabled(!Profile.bCanModify);
                if (ImGui::SmallButton("Delete")) Changed |= Profiles->RemoveProfile(Profile.Name);
                ImGui::EndDisabled(); ImGui::PopID();
            }
            ImGui::SetNextWindowSize(ImVec2(590, 590), ImGuiCond_FirstUseEver);
            if (ImGui::BeginPopupModal("Collision Preset", nullptr))
            {
                ImGui::BeginDisabled(!NewProfile && !Editing.bCanModify); ImGui::InputText("Name", Name, sizeof(Name)); ImGui::EndDisabled();
                int Enabled = Editing.CollisionEnabled; if (ImGui::Combo("Collision Enabled", &Enabled, EnabledNames, 6)) Editing.CollisionEnabled = static_cast<ECollisionEnabled::Type>(Enabled);
                DrawObjectType(Editing.ObjectType); ImGui::InputTextMultiline("Description", Description, sizeof(Description), ImVec2(-1, 55)); DrawResponses(Editing.ResponseToChannels);
                if (ImGui::Button("Accept"))
                {
                    const FName PreviousName = Editing.Name;
                    auto Updated = Editing; Updated.Name = FName(Name); Updated.HelpMessage = Description; FCollisionResponseTemplate Existing;
                    const bool Renamed = !NewProfile && Updated.Name != PreviousName;
                    if (((!NewProfile && !Renamed) || !Profiles->GetProfileTemplate(Updated.Name, Existing)) && Profiles->SetProfile(Updated))
                    {
                        if (Renamed)
                        {
                            Profiles->RemoveProfile(PreviousName);
                            if (_World) for (auto* Level : _World->GetLevels()) for (const auto& Actor : Level->GetActors()) for (auto* Component : Actor->GetComponents())
                                if (auto* Primitive = dynamic_cast<UPrimitiveComponent*>(Component); Primitive && Primitive->GetCollisionProfileName() == PreviousName) Primitive->SetCollisionProfileName(Updated.Name, false);
                        }
                        Changed = true; ImGui::CloseCurrentPopup(); __CollisionSettingsError = {};
                    }
                    else __CollisionSettingsError = TEXT("A preset needs a unique name and a valid object channel.");
                }
                ImGui::SameLine(); if (ImGui::Button("Cancel")) ImGui::CloseCurrentPopup();
                if (!__CollisionSettingsError.IsEmpty()) ImGui::TextWrapped("%s", __CollisionSettingsError.ToUtf8().c_str());
                ImGui::EndPopup();
            }
        }
        if (Changed)
        {
            if (_World) for (auto* Level : _World->GetLevels()) for (const auto& Actor : Level->GetActors()) for (auto* Component : Actor->GetComponents())
                if (auto* Primitive = dynamic_cast<UPrimitiveComponent*>(Component); Primitive && Primitive->GetCollisionProfileName() != FName(TEXT("Custom")))
                {
                    FCollisionResponseTemplate Profile;
                    Primitive->SetCollisionProfileName(Profiles->GetProfileTemplate(Primitive->GetCollisionProfileName(), Profile) ? Profile.Name : FName(TEXT("Custom")), false);
                }
            if (!Paths || !Profiles->SaveConfig(Paths->GetProjectDirectory() / TEXT("Config") / TEXT("CollisionProfiles.cfg"))) __CollisionSettingsError = TEXT("Collision settings could not be saved.");
            else __CollisionSettingsError = {};
        }
        if (!__CollisionSettingsError.IsEmpty()) ImGui::TextWrapped("%s", __CollisionSettingsError.ToUtf8().c_str());
    });
}

void FEditorImGui::DrawCollisionPreview()
{
    if (!__ShowCollisionPreview) return;
    ImGui::SetNextWindowSize(ImVec2(820, 680), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Collision Test", &__ShowCollisionPreview)) { ImGui::End(); return; }
    auto Start = [&](bool _3D)
    {
        __CollisionPreviewWorld = std::make_unique<UWorld>(); __CollisionPreview3D = _3D;
        __CollisionPreviewHits = __CollisionPreviewOverlaps = 0; __CollisionPreviewRunning = true;
        auto* FloorActor = __CollisionPreviewWorld->SpawnActor<AActor>(); auto* Floor = FloorActor->CreateInstanceComponent<UBoxComponent>(); FloorActor->SetRootComponent(Floor);
        Floor->SetBoxExtent(FVector(300, _3D ? 180 : 5, 10)); Floor->SetWorldLocation(FVector(0, 0, -10)); Floor->SetCollisionProfileName(FName(TEXT("BlockAll")));
        for (int Index = 0; Index < 3; ++Index)
        {
            auto* Actor = __CollisionPreviewWorld->SpawnActor<AActor>(); UPrimitiveComponent* Component = nullptr;
            if (Index == 0) Component = Actor->CreateInstanceComponent<UBoxComponent>();
            else if (Index == 1) Component = Actor->CreateInstanceComponent<USphereComponent>();
            else Component = Actor->CreateInstanceComponent<UCapsuleComponent>();
            Actor->SetRootComponent(Component); Component->SetWorldLocation(FVector((Index - 1) * 130, _3D ? (Index - 1) * 35 : 0, 250 + Index * 50));
            Component->SetCollisionProfileName(FName(TEXT("PhysicsActor"))); Component->SetNotifyRigidBodyCollision(true);
            if (!_3D) Component->BodyInstance.SetDOFLock(EDOFMode::XZPlane);
            Component->SetSimulatePhysics(true);
            Component->OnComponentHit.AddLambda([this](UPrimitiveComponent*, AActor*, UPrimitiveComponent*, FVector, const FHitResult&) { ++__CollisionPreviewHits; });
        }
        auto* TriggerActor = __CollisionPreviewWorld->SpawnActor<AActor>(); auto* Trigger = TriggerActor->CreateInstanceComponent<UBoxComponent>(); TriggerActor->SetRootComponent(Trigger);
        Trigger->SetBoxExtent(FVector(250, _3D ? 150 : 10, 15)); Trigger->SetWorldLocation(FVector(0, 0, 130)); Trigger->SetCollisionProfileName(FName(TEXT("Trigger")));
        Trigger->OnComponentBeginOverlap.AddLambda([this](UPrimitiveComponent*, AActor*, UPrimitiveComponent*, int32, bool, const FHitResult&) { ++__CollisionPreviewOverlaps; });
    };
    if (ImGui::Button("Run 2D (XZ Plane)")) Start(false);
    ImGui::SameLine(); if (ImGui::Button("Run 3D")) Start(true);
    ImGui::SameLine(); if (ImGui::Button(__CollisionPreviewRunning ? "Pause" : "Resume")) __CollisionPreviewRunning = !__CollisionPreviewRunning;
    if (__CollisionPreviewWorld && __CollisionPreviewRunning) __CollisionPreviewWorld->Tick(ImGui::GetIO().DeltaTime);
    ImGui::Text("Hit events: %d    Begin Overlap events: %d", __CollisionPreviewHits, __CollisionPreviewOverlaps);
    ImGui::TextUnformatted("Blue: dynamic bodies   Gray: floor   Green: Query Only trigger");
    const auto Available = ImGui::GetContentRegionAvail(); const ImVec2 Size((std::max)(Available.x, 100.f), (std::max)(Available.y, 200.f));
    const auto Origin = ImGui::GetCursorScreenPos();
    ImGui::InvisibleButton("##PhysicsPreview", Size);
    auto* Draw = ImGui::GetWindowDrawList(); Draw->AddRectFilled(Origin, ImVec2(Origin.x + Size.x, Origin.y + Size.y), IM_COL32(22, 24, 28, 255));
    Draw->PushClipRect(Origin, ImVec2(Origin.x + Size.x, Origin.y + Size.y), true);
    const float Scale = (std::min)(Size.x / 750.f, Size.y / 520.f);
    auto Project = [&](FVector _Point)
    {
        const double X = __CollisionPreview3D ? _Point.X * .85 - _Point.Y * .55 : _Point.X;
        const double Y = __CollisionPreview3D ? _Point.X * .18 + _Point.Y * .3 - _Point.Z : -_Point.Z;
        return ImVec2(Origin.x + Size.x * .5f + float(X) * Scale, Origin.y + Size.y * .84f + float(Y) * Scale);
    };
    if (__CollisionPreviewWorld) for (const auto& Actor : __CollisionPreviewWorld->GetPersistentLevel()->GetActors())
    {
        auto* Component = dynamic_cast<UPrimitiveComponent*>(Actor->GetRootComponent()); if (!Component) continue;
        const auto Shape = Component->GetCollisionShape(); const auto Transform = Component->GetComponentTransform();
        const ImU32 Color = Component->IsSimulatingPhysics() ? IM_COL32(90,170,255,255) : Component->GetCollisionEnabled() == ECollisionEnabled::QueryOnly ? IM_COL32(80,220,120,255) : IM_COL32(160,160,170,255);
        auto Line = [&](FVector _A, FVector _B) { Draw->AddLine(Project(Transform.TransformPosition(_A)), Project(Transform.TransformPosition(_B)), Color, 1.8f); };
        if (Shape.IsBox())
        {
            const auto Extent = Shape.GetBox(); FVector Corners[8];
            for (int Index = 0; Index < 8; ++Index) Corners[Index] = FVector((Index & 1) ? Extent.X : -Extent.X, (Index & 2) ? Extent.Y : -Extent.Y, (Index & 4) ? Extent.Z : -Extent.Z);
            for (int Index = 0; Index < 8; ++Index) for (int Axis = 0; Axis < 3; ++Axis) if (!(Index & (1 << Axis))) Line(Corners[Index], Corners[Index | (1 << Axis)]);
        }
        else
        {
            const float Radius = Shape.IsSphere() ? Shape.GetSphereRadius() : Shape.GetCapsuleRadius();
            const float Offset = Shape.IsSphere() ? 0 : Shape.GetCapsuleHalfHeight() - Radius;
            for (int Plane = 0; Plane < (__CollisionPreview3D ? 3 : 1); ++Plane) for (int Index = 0; Index < 48; ++Index)
            {
                auto Point = [&](int _Index)
                {
                    const double Angle = _Index * 6.28318530718 / 48; const double X = std::cos(Angle) * Radius; const double Y = std::sin(Angle) * Radius;
                    return Plane == 2 ? FVector(X, Y, 0) : Plane == 1 ? FVector(0, X, Y + (Y >= 0 ? Offset : -Offset)) : FVector(X, 0, Y + (Y >= 0 ? Offset : -Offset));
                };
                Line(Point(Index), Point(Index + 1));
            }
        }
    }
    Draw->PopClipRect(); ImGui::End();
}
