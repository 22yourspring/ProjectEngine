param([ValidateSet('debug', 'release', 'all')][string]$Configuration = 'all')
$ErrorActionPreference = 'Stop'
$CMake = Get-Command cmake -ErrorAction SilentlyContinue
if ($CMake) { $CMakePath = $CMake.Source }
else {
    $VsWhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio/Installer/vswhere.exe'
    if (!(Test-Path -LiteralPath $VsWhere)) { throw 'Install Visual Studio 2022 C++ tools and CMake first.' }
    $VsPath = & $VsWhere -latest -version '[17,18)' -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
    if (!$VsPath) { throw 'Visual Studio 2022 C++ tools were not found.' }
    $CMakePath = Join-Path $VsPath 'Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/cmake.exe'
}
if (!(Test-Path -LiteralPath $CMakePath)) { throw 'CMake was not found.' }
$BuildPath = Join-Path $PSScriptRoot 'build/vs2022-x64'
& $CMakePath -S $PSScriptRoot -B $BuildPath -G 'Visual Studio 17 2022' -A x64 -T v143
if ($LASTEXITCODE -ne 0) { throw 'PhysX configuration failed.' }
$Configurations = if ($Configuration -eq 'all') { @('debug', 'release') } else { @($Configuration) }
foreach ($Selected in $Configurations) {
    & $CMakePath --build $BuildPath --config $Selected --parallel 2
    if ($LASTEXITCODE -ne 0) { throw "PhysX $Selected build failed." }
}
