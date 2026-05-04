# Downloads libmpv prebuilt dev package (shinchiro build) and installs it to extern/libmpv-prebuilt/.
# Run once before building. build.ps1 calls this automatically if the prebuilt is missing.

[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'

$LIBMPV_URL = "https://sourceforge.net/projects/mpv-player-windows/files/libmpv/mpv-dev-x86_64-20260419-git-06f4ce7.7z/download"
$REPO_ROOT  = Split-Path -Parent $PSScriptRoot
$DEST_DIR   = Join-Path $REPO_ROOT "extern\libmpv-prebuilt"
$MARKER     = Join-Path $DEST_DIR "lib\mpv.lib"
$TEMP_7Z    = Join-Path $env:TEMP "libmpv-prebuilt-download.7z"
$TEMP_DIR   = Join-Path $env:TEMP "libmpv-prebuilt-extract"

if (Test-Path $MARKER) {
    Write-Host "[libmpv] Already installed — skipping." -ForegroundColor Green
    exit 0
}

Write-Host "[libmpv] Downloading libmpv dev x86_64 from SourceForge..." -ForegroundColor Cyan

# Use curl with -L to follow SourceForge redirects (Invoke-WebRequest fails on JS redirects)
$curlExe = Get-Command curl.exe -ErrorAction SilentlyContinue
if ($curlExe) {
    & curl.exe -L -o $TEMP_7Z $LIBMPV_URL --fail --retry 3
    if ($LASTEXITCODE -ne 0) {
        Write-Error "[libmpv] Download failed."
        exit 1
    }
} else {
    Invoke-WebRequest -Uri $LIBMPV_URL -OutFile $TEMP_7Z -UseBasicParsing -MaximumRedirection 10
}

Write-Host "[libmpv] Extracting..." -ForegroundColor Cyan
if (Test-Path $TEMP_DIR) { Remove-Item $TEMP_DIR -Recurse -Force }
New-Item -ItemType Directory -Path $TEMP_DIR | Out-Null

# Use 7z if available
$sevenZip = $null
foreach ($candidate in @("7z", "C:\Program Files\7-Zip\7z.exe", "C:\Program Files (x86)\7-Zip\7z.exe")) {
    $found = Get-Command $candidate -ErrorAction SilentlyContinue
    if ($found) { $sevenZip = $found.Source; break }
}
if ($sevenZip) {
    & $sevenZip x $TEMP_7Z "-o$TEMP_DIR" -y | Out-Null
} else {
    Write-Error "[libmpv] 7-Zip not found. Install 7-Zip (https://7-zip.org/) or add 7z to PATH."
    exit 1
}

foreach ($dir in @("bin", "include\mpv", "lib")) {
    $d = Join-Path $DEST_DIR $dir
    if (-not (Test-Path $d)) { New-Item -ItemType Directory -Path $d | Out-Null }
}

Write-Host "[libmpv] Copying files to extern/libmpv-prebuilt/..." -ForegroundColor Cyan

# Copy DLL
Copy-Item (Join-Path $TEMP_DIR "libmpv-2.dll") (Join-Path $DEST_DIR "bin\") -Force

# Copy headers
Copy-Item (Join-Path $TEMP_DIR "include\mpv\*") (Join-Path $DEST_DIR "include\mpv\") -Force

# Generate MSVC .lib from DLL exports using dumpbin + lib.exe
$dllFile = Join-Path $DEST_DIR "bin\libmpv-2.dll"
$defFile = Join-Path $TEMP_DIR "libmpv.def"
$libFile = Join-Path $DEST_DIR "lib\mpv.lib"

Write-Host "[libmpv] Generating mpv.lib from DLL exports..." -ForegroundColor Cyan

# Find MSVC tools via vswhere
$dumpbinExe = $null
$libExe = $null
$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
if (Test-Path $vswhere) {
    $vsPath = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
    $vcToolsVersion = Get-Content "$vsPath\VC\Auxiliary\Build\Microsoft.VCToolsVersion.default.txt" -ErrorAction SilentlyContinue
    if ($vcToolsVersion) {
        $vcToolsVersion = $vcToolsVersion.Trim()
        $toolDir = "$vsPath\VC\Tools\MSVC\$vcToolsVersion\bin\Hostx64\x64"
        $candidate = "$toolDir\lib.exe"
        if (Test-Path $candidate) { $libExe = $candidate }
        $candidate = "$toolDir\dumpbin.exe"
        if (Test-Path $candidate) { $dumpbinExe = $candidate }
    }
}

if ($dumpbinExe -and $libExe) {
    # Extract mpv_ exports from DLL
    $exports = & $dumpbinExe /exports $dllFile 2>&1
    $mpvExports = $exports | Where-Object { $_ -match '^\s+\d+\s+[0-9A-Fa-f]+\s+[0-9A-Fa-f]+\s+(mpv_\S+)' } |
        ForEach-Object { $Matches[1] }

    $defContent = "LIBRARY libmpv-2`nEXPORTS`n" + ($mpvExports -join "`n")
    Set-Content -Path $defFile -Value $defContent

    & $libExe /def:$defFile /out:$libFile /MACHINE:X64 2>&1 | Out-Null
    Write-Host "[libmpv] Generated mpv.lib with $($mpvExports.Count) exports" -ForegroundColor Green
} else {
    Write-Warning "[libmpv] MSVC tools not found — could not generate mpv.lib. Build may fail."
}

Remove-Item $TEMP_7Z  -Force -ErrorAction SilentlyContinue
Remove-Item $TEMP_DIR -Recurse -Force -ErrorAction SilentlyContinue

if (Test-Path $MARKER) {
    Write-Host "[libmpv] Done." -ForegroundColor Green
} else {
    Write-Error "[libmpv] Installation may be incomplete — mpv.lib not found at $MARKER"
    exit 1
}
