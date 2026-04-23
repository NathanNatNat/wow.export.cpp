# Downloads FFmpeg prebuilt LGPL shared x64 (BtbN) and installs it to extern/ffmpeg-prebuilt/.
# Run once before building. build.ps1 calls this automatically if the prebuilt is missing.

[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'

$FFMPEG_URL = "https://github.com/BtbN/FFmpeg-Builds/releases/download/latest/ffmpeg-master-latest-win64-lgpl-shared.zip"
$REPO_ROOT  = Split-Path -Parent $PSScriptRoot
$DEST_DIR   = Join-Path $REPO_ROOT "extern\ffmpeg-prebuilt"
$MARKER     = Join-Path $DEST_DIR "lib\avcodec.lib"
$TEMP_ZIP   = Join-Path $env:TEMP "ffmpeg-prebuilt-download.zip"
$TEMP_DIR   = Join-Path $env:TEMP "ffmpeg-prebuilt-extract"

if (Test-Path $MARKER) {
    Write-Host "[ffmpeg] Already installed — skipping." -ForegroundColor Green
    exit 0
}

Write-Host "[ffmpeg] Downloading FFmpeg LGPL shared x64 from BtbN..." -ForegroundColor Cyan
Invoke-WebRequest -Uri $FFMPEG_URL -OutFile $TEMP_ZIP -UseBasicParsing

Write-Host "[ffmpeg] Extracting..." -ForegroundColor Cyan
if (Test-Path $TEMP_DIR) { Remove-Item $TEMP_DIR -Recurse -Force }
Expand-Archive -Path $TEMP_ZIP -DestinationPath $TEMP_DIR

$extracted = Get-ChildItem $TEMP_DIR -Directory | Select-Object -First 1
if (-not $extracted) {
    Write-Error "[ffmpeg] Could not find extracted FFmpeg directory inside zip."
    exit 1
}

foreach ($dir in @("bin", "include", "lib")) {
    $d = Join-Path $DEST_DIR $dir
    if (-not (Test-Path $d)) { New-Item -ItemType Directory -Path $d | Out-Null }
}

Write-Host "[ffmpeg] Copying files to extern/ffmpeg-prebuilt/..." -ForegroundColor Cyan
Copy-Item (Join-Path $extracted.FullName "bin\av*.dll")        (Join-Path $DEST_DIR "bin\") -Force
Copy-Item (Join-Path $extracted.FullName "bin\sw*.dll")        (Join-Path $DEST_DIR "bin\") -Force
Copy-Item (Join-Path $extracted.FullName "bin\postproc*.dll")  (Join-Path $DEST_DIR "bin\") -Force -ErrorAction SilentlyContinue
Copy-Item (Join-Path $extracted.FullName "lib\*.lib")          (Join-Path $DEST_DIR "lib\") -Force
Copy-Item (Join-Path $extracted.FullName "include\*")          (Join-Path $DEST_DIR "include\") -Recurse -Force

Remove-Item $TEMP_ZIP  -Force -ErrorAction SilentlyContinue
Remove-Item $TEMP_DIR  -Recurse -Force -ErrorAction SilentlyContinue

Write-Host "[ffmpeg] Done." -ForegroundColor Green
