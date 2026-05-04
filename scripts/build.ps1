[CmdletBinding()]
param(
    [switch]$Clean,
    [ValidateSet('debug', 'release', 'relwithdebinfo')]
    [string]$Config = 'debug'
)

$ErrorActionPreference = 'Stop'

# Ensure FFmpeg prebuilt is present before configuring CMake.
$ffmpegMarker = Join-Path $PSScriptRoot "..\extern\ffmpeg-prebuilt\lib\avcodec.lib"
if (-not (Test-Path $ffmpegMarker)) {
    Write-Host "[build] FFmpeg prebuilt not found — running setup-ffmpeg.ps1..." -ForegroundColor Yellow
    & (Join-Path $PSScriptRoot "setup-ffmpeg.ps1")
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}

# Ensure libmpv prebuilt is present before configuring CMake.
$libmpvMarker = Join-Path $PSScriptRoot "..\extern\libmpv-prebuilt\lib\mpv.lib"
if (-not (Test-Path $libmpvMarker)) {
    Write-Host "[build] libmpv prebuilt not found — running setup-libmpv.ps1..." -ForegroundColor Yellow
    & (Join-Path $PSScriptRoot "setup-libmpv.ps1")
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}

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
