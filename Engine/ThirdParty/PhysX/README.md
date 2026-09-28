# PhysX 5.4.2

NVIDIA PhysX CPU SDK를 프로젝트 안에 보관한다. 별도 PhysX 설치, CUDA 설치, NVIDIA GPU 없이 사용할 수 있다.

## 구성

- `sdk/physx`: 헤더와 정적 라이브러리 재빌드에 필요한 소스
- `lib/Win64/debug`, `lib/Win64/release`: Windows x64 / VS2022 v143 정적 라이브러리
- `PhysX.props`: 상대 경로 기반 Visual C++ include/link 설정
- `Build.ps1`: 소스로부터 라이브러리 재빌드
- `LICENSE.md`: 배포 시 보존할 upstream 라이선스

## 다른 컴퓨터에서 사용

이 디렉터리를 통째로 프로젝트와 함께 복사한다. C++ 프로젝트 빌드에는 Visual Studio 2022의 C++ 도구와 Windows SDK가 필요하다. PhysX 전용 설치 프로그램이나 전역 환경 변수 설정은 필요 없다.

라이브러리는 Debug `/MDd`, Release `/MD`로 빌드한다. 이를 사용하는 프로젝트도 같은 CRT 설정을 사용한다. 정적 PhysX이므로 PhysX DLL은 필요 없지만, `/MD` 실행 파일 배포 시에는 일반적인 MSVC 런타임 요구 사항이 남는다. Debug 바이너리는 개발 환경용이다.

현재 UE 프로젝트가 `PhysX.props`를 Import하여 라이브러리를 연결한다. Client와 Editor는 UE의 충돌·물리 인터페이스를 통해 사용한다. 다른 프로젝트에 연결할 때는 `.vcxproj`의 `Microsoft.Cpp.props` 이후에 프로젝트 위치에 맞는 상대 경로로 `PhysX.props`를 Import한다.

```xml
<Import Project="상대경로\Engine\ThirdParty\PhysX\PhysX.props" />
```

```cpp
#include <PxPhysicsAPI.h>
```

지원 구성은 x64 Debug/Release이다. Win32용 라이브러리는 포함하지 않는다. 다른 OS/아키텍처/호환되지 않는 컴파일러에서는 해당 환경용 재빌드와 설정이 필요하다. Linux CMake 분기는 검증하지 않았다.

## 재빌드

PowerShell에서 이 디렉터리를 기준으로 실행한다. CMake는 PATH 또는 VS2022에 포함된 버전을 찾는다. 소스와 빌드 설정은 로컬에 있으므로 재빌드 중 SDK 다운로드는 없다.

```powershell
powershell -ExecutionPolicy Bypass -File .\Build.ps1
powershell -ExecutionPolicy Bypass -File .\Build.ps1 -Configuration release
```

`build/`는 임시 생성물이다. 디렉터리를 옮겨 재빌드할 때는 이전 절대 경로를 저장한 CMake 캐시가 남지 않도록 `build/`를 제외하고 복사한다. `sdk/`와 `lib/`는 함께 보관한다.

## 출처와 변경

- 저장소: https://github.com/NVIDIA-Omniverse/PhysX
- 태그: `106.1-physx-5.4.2`
- 커밋: `19542b61c4fee8e5aaa97f10455933488383d092`
- 포함: `physx/include`, `source`, `compiler`, `buildtools`, `pvdruntime`
- 제외: 샘플, 문서, GPU 바이너리, 외부 패키지 자동 다운로드 도구
- `sdk/physx/source/compiler/cmake/windows/CMakeLists.txt`의 외부 DLL 복사 조건 두 곳에 `AND NOT PHYSX_PORTABLE_CPU_ONLY`를 추가했다. CPU 전용 빌드에서 PhysXDevice/FreeGLUT/GPU DLL 복사를 생략하기 위한 변경이다.
- `sdk/physx/include/PxConfig.h`는 정적 라이브러리 설정으로 CMake가 생성한다.

GPU 시뮬레이션은 이 패키지의 지원 범위에 포함하지 않는다. PhysX를 UE.dll에 정적으로 연결할 수 있으나, Hot Reload 시 물리 객체의 수명과 상태 이전은 엔진에서 별도로 설계해야 한다.
