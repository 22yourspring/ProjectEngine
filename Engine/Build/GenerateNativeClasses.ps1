param([Parameter(Mandatory=$true)][string]$ProjectFile, [Parameter(Mandatory=$true)][string]$OutputFile, [Parameter(Mandatory=$true)][string]$Module)
$ErrorActionPreference = 'Stop'
if($Module -notmatch '^[A-Za-z_][A-Za-z0-9_]*$'){throw 'Invalid module name'}
$projectPath = [IO.Path]::GetFullPath($ProjectFile)
$directory = [IO.Path]::GetDirectoryName($projectPath)
[xml]$projectXml = [IO.File]::ReadAllText($projectPath)
$classes = [Collections.Generic.List[object]]::new()
$names = [Collections.Generic.HashSet[string]]::new()
$includes = [Collections.Generic.HashSet[string]]::new()
foreach($item in $projectXml.SelectNodes('//*[local-name()="ClInclude"]')) {
    $header = [IO.Path]::GetFullPath((Join-Path $directory $item.Include))
    $code = [IO.File]::ReadAllText($header)
    $code = [regex]::Replace($code, '(?s)/\*.*?\*/|//[^\r\n]*|"(?:\\.|[^"\\])*"', ' ')
    foreach($match in [regex]::Matches($code, '\bUCLASS\s*\((?<flags>(?:[^()]|\([^()]*\))*)\)\s*class\s+(?:\w+_API\s+)?(?<name>[AU]\w+)\b[^;{]*\{')) {
        if($match.Groups['flags'].Value -match '\b(Abstract|NotPlaceable)\b'){continue}
        $name = $match.Groups['name'].Value
        if(!$names.Add($name)){throw "Duplicate reflected class: $name"}
        [void]$includes.Add($header.Replace('\','/'))
        $classes.Add([pscustomobject]@{Name=$name; Path="/Script/$Module.$($name.Substring(1))"})
    }
}
$lines = [Collections.Generic.List[string]]::new()
$lines.Add('#include "UE/NativeClassRegistration.h"')
foreach($header in ($includes | Sort-Object)){$lines.Add('#include "' + $header + '"')}
$lines.Add('extern "C" __declspec(dllexport) void RegisterNativeClasses()')
$lines.Add('{')
foreach($class in $classes){$lines.Add('    TNativeClassRegistration<' + $class.Name + '>::Register(TEXT("' + $class.Path + '"));')}
$lines.Add('}')
$lines.Add('extern "C" __declspec(dllexport) void UnregisterNativeClasses()')
$lines.Add('{')
foreach($class in $classes){$lines.Add('    TNativeClassRegistration<' + $class.Name + '>::Unregister(TEXT("' + $class.Path + '"));')}
$lines.Add('}')
$outputPath = [IO.Path]::GetFullPath($OutputFile)
[void][IO.Directory]::CreateDirectory([IO.Path]::GetDirectoryName($outputPath))
$content = ($lines -join "`r`n") + "`r`n"
if(!(Test-Path -LiteralPath $outputPath) -or [IO.File]::ReadAllText($outputPath) -cne $content){[IO.File]::WriteAllText($outputPath,$content,[Text.UTF8Encoding]::new($false))}
