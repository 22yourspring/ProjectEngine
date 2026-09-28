# UnrealEngine

Unreal Engine의 런타임 구조에서 영감을 받아 직접 설계한 C++ 기반 Custom Engine Runtime Framework입니다.  
Engine Loop, World, Actor/Component, Tick, Subsystem, Delegate, Input, Update/Render Thread 분리 및 Rendering Abstraction을 하나의 실행 가능한 구조로 연결하는 데 초점을 두었습니다.

## 처음 실행하는 방법

1. Visual Studio 2022에서 `UnrealEngine.sln`을 엽니다.
2. Solution Configuration을 `Debug`, Platform을 `x64`로 선택합니다.
3. Solution Explorer에서 `Client`를 **시작 프로젝트로 설정**합니다.
4. `Client`를 빌드하고 실행합니다.

권장 환경은 Visual Studio 2022, Desktop development with C++ workload, Windows SDK입니다. 프로젝트는 C++20 설정을 사용합니다. 외부 WIL 헤더는 `Engine/ThirdParty/WIL`에 포함되어 있습니다.

`Editor`를 시작 프로젝트로 선택하면 ImGui 편집기에서 Level과 Actor/Component를 편집하고 Play/Stop으로 확인할 수 있습니다. 충돌 예제는 `Game/Project/Content/Levels/PhysicsGround.umap`에 있습니다.

## 가장 먼저 볼 코드

### 1. 엔진의 시작과 종료

- `Game/Programs/Client/main.cpp`
  - Win32 진입점
  - Engine Loop 초기화 및 종료
  - Project 초기화와 OS Message Loop 연결
- `Engine/UE/LaunchEngineLoop.cpp`
  - Engine / Subsystem Bootstrap
  - Update Thread와 Render Thread 생성
  - Runtime 전체 Initialize / Shutdown 순서 관리

먼저 위 두 파일을 보면 애플리케이션이 엔진을 실행하고 종료하는 전체 흐름을 빠르게 파악할 수 있습니다.

### 2. Runtime Object 구조

- `Engine/UE/Engine.h`, `Engine/UE/Engine.cpp`
- `Engine/UE/World.h`, `Engine/UE/World.cpp`
- `Engine/UE/Level.h`, `Engine/UE/Level.cpp`
- `Engine/UE/Object.h`
- `Engine/UE/Actor.h`, `Engine/UE/Actor.inl`
- `Engine/UE/ActorComponent.h`

`UEngine → UWorld → ULevel → AActor → UActorComponent`로 이어지는 객체 계층과 소유권, `SpawnActor()`, `NewObject()`, `CreateDefaultSubobject()`의 Generic 생성 흐름을 확인할 수 있습니다.

### 3. Tick / Delegate / Subsystem

- `Engine/UE/Tickable.h`, `Engine/UE/TickTaskManager.cpp`
  - Delta Time 및 Tick Group 기반 Update Pipeline
- `Engine/UE/Delegate.h`, `Engine/UE/MulticastDelegate.h`
  - Lambda / Member / Static Function Binding
  - Dispatch 중 Remove 안전성을 위한 지연 정리
- `Engine/UE/Subsystem.h`, `Engine/UE/Engine.inl`
  - `std::type_index` 기반 Generic Registry
  - Subsystem 생성, 조회 및 Lazy Initialization

### 4. Thread / Rendering 경계

- `Engine/UE/LaunchEngineLoop.cpp`
  - Update Thread / Render Thread 분리
- `Engine/UE/RenderEngineSystem.cpp`
  - Viewport Frame 시작, Scene Rendering, Extension 실행, Present
- `Engine/UE/Scene.cpp`, `Engine/UE/PrimitiveSceneProxy.h`
  - Runtime Component와 Render Data의 경계
- `Engine/UE/AppTimeRenderProxy.*`
  - Render Proxy를 통한 Thread 간 상태 전달 예시
- `Engine/UE/DynamicRHI.*`, `Engine/UE/Windows/WindowsGDIRHI.*`
  - Rendering Interface와 Windows GDI Backend 분리
- `Engine/UE/SceneViewExtension.h`
  - Core 수정 없이 Rendering 기능을 추가하는 Extension Point

### 5. Gameplay Layer 사용 예시

- `Game/Project/Project.cpp`
  - World에서 Player / PlayerController Spawn
  - Possess 및 Input Mapping 저장·복원
- `Game/Project/Player.cpp`
  - `CreateDefaultSubobject()` 기반 Component 구성
  - Tick과 Axis Input을 사용한 Actor 이동
- `Game/Programs/Client/main.cpp`
  - Win32 UI 기반 Input Mapping 편집 기능

이 부분은 Engine Core의 API가 실제 Gameplay Module에서 어떻게 사용되는지 보여줍니다.

## 프로젝트 구성

```text
UnrealEngine.sln
├─ Engine/UE                     Engine Runtime DLL
├─ Engine/ThirdParty/WIL         Windows 리소스 RAII 지원 Header
├─ Game/Project           Gameplay DLL와 사용 예시
└─ Game/Programs
   ├─ Client                     핵심 실행 애플리케이션
   ├─ Editor                     ImGui 기반 Level·Asset·Collision 편집기
   └─ SharedLibraries            Program 공통 Include 설정
```

## 구현 사항

- 명시적인 Runtime Lifecycle: Initialize / Update / PostUpdate / Render / Shutdown
- UObject 스타일 Object Hierarchy와 Actor/Component Composition
- Tick Group 기반 Frame Update
- Single / Multicast Delegate와 Dispatch-safe Remove
- Type Registry 기반 Generic Subsystem Framework
- Update / Render Thread 분리와 동기화
- Render Proxy 기반 Thread-safe Data Boundary
- Dynamic RHI Interface와 Windows GDI Backend
- SceneViewExtension 기반 Rendering 확장
- RAII와 Smart Pointer 기반 명시적 Ownership
- Template, Variadic Template, Perfect Forwarding 및 Compile-time Type Constraint

## 충돌과 PhysX

- Box·Sphere·Capsule 충돌, Channel/Profile, Trace·Sweep·Overlap과 물리 시뮬레이션을 지원합니다.
- `FBodyInstance`, `FPhysScene_Chaos`, `FPhysicsInterface`는 UE의 호출 계층을 참고한 자체 래퍼입니다. 실제 물리 계산은 NVIDIA PhysX 5.4.2가 담당합니다.
- `Engine/ThirdParty/PhysX`에 SDK 소스와 Windows x64 / VS2022 v143 Debug·Release 정적 라이브러리를 포함합니다. 별도 PhysX·CUDA 설치나 NVIDIA GPU는 필요하지 않습니다.
- Debug는 `/MDd`, Release는 `/MD`를 사용합니다. 다른 OS·아키텍처에서는 라이브러리 재빌드가 필요합니다.
- SDK 구성과 재빌드 방법은 [PhysX README](Engine/ThirdParty/PhysX/README.md)를 참고하세요.
- Complex Triangle Mesh 충돌과 UE의 전체 Chaos·Reflection·Hot Reload 기능은 구현 범위에 포함되지 않습니다.

## 수업 기준점

현재 소스·설정·Content와 PhysX 의존성을 커밋으로 보관합니다. 수업 대본 작성 시 시작 커밋과 종료 커밋 사이의 전체 변경을 비교하여 버그 수정도 포함합니다. 빌드 결과물·자동 저장본·개인 에디터 배치는 제외합니다.
