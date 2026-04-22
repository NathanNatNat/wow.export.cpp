[CmdletBinding()]
param(
    [switch]$Clean,
    [ValidateSet('debug', 'release', 'relwithdebinfo')]
    [string]$Config = 'debug'
)

$ErrorActionPreference = 'Stop'

$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
if (-not (Test-Path $vswhere)) {
    Write-Error "vswhere.exe not found — Visual Studio is not installed"
    exit 1
}

$vsPath = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (-not $vsPath) {
    Write-Error "No Visual Studio installation with MSVC x64 tools found"
    exit 1
}

$configPreset = "windows-msvc-$Config"
$buildPreset  = "windows-$Config"
$outDir       = "out/build/$configPreset"

if ($Clean -and (Test-Path $outDir)) {
    Write-Host "Cleaning $outDir ..."
    Remove-Item -Recurse -Force $outDir
}

$vcvars = "$vsPath\Common7\Tools\VsDevCmd.bat"
$chain  = "`"$vcvars`" -arch=amd64 -no_logo && cmake --preset $configPreset && cmake --build --preset $buildPreset --parallel"

cmd /c $chain
exit $LASTEXITCODE
