# CHESS LEGENDS: ARCANE WARS build entry point for Windows PowerShell.
#
#   .\build.ps1                 -> regenerate assets + build release\ChessLegends.gba
#   .\build.ps1 rom             -> build the ROM only (generated assets must exist)
#   .\build.ps1 assets          -> regenerate graphics / audio from assets_source\
#   .\build.ps1 tests [filter]  -> build + run the host-side test suite
#   .\build.ps1 sim [args]      -> build + run the AI / balance simulator
#   .\build.ps1 harness         -> build the headless mGBA test harness
#   .\build.ps1 emu <args>      -> run an emulator scenario (see tools\harness\cl_harness.c)
#   .\build.ps1 verify          -> full release verification
#   .\build.ps1 clean           -> remove build output
#
# GNU make / gcc cannot cope with spaces and non-ASCII characters in the project path, so the project folder is
# mapped to an ASCII drive letter with `subst` (session-only, no admin rights, no global settings changed).
#
# Environment overrides:
#   CL_MSYS2  MSYS2 root containing the devkitPro / Butano installation (default C:\msys64)
#   CL_DRIVE  Drive letter used for the ASCII mapping                      (default Q:)
param(
    [Parameter(Position = 0)][string]$Command = "all",
    [Parameter(Position = 1, ValueFromRemainingArguments = $true)][string[]]$Rest
)

$ErrorActionPreference = "Stop"
$msys = if ($env:CL_MSYS2) { $env:CL_MSYS2 } else { "C:\msys64" }
$drive = if ($env:CL_DRIVE) { $env:CL_DRIVE } else { "Q:" }
$root = $PSScriptRoot

$bash = Join-Path $msys "usr\bin\bash.exe"
if (-not (Test-Path $bash)) {
    Write-Error "MSYS2 not found at $msys. See docs\BUILDING.md."
    exit 2
}

# Map the project to an ASCII-only drive letter unless this script already runs from such a mapping.
$mapped = $false
if (Test-Path "$drive\build.ps1") {
    if ((Get-FileHash "$drive\build.ps1").Hash -eq (Get-FileHash $PSCommandPath).Hash) { $mapped = $true }
    else { Write-Error "$drive is mapped to another folder. Set CL_DRIVE to a free letter."; exit 2 }
}
if (-not $mapped) {
    if (Test-Path "$drive\") { Write-Error "$drive is in use. Set CL_DRIVE to a free letter."; exit 2 }
    subst $drive $root
}

$letter = $drive.Substring(0, 1).ToLower()
$env:MSYSTEM = "MSYS"
$env:CHERE_INVOKING = "1"
$argString = ""
if ($Rest) { $argString = ($Rest | ForEach-Object { "'" + ($_ -replace "'", "'\''") + "'" }) -join " " }
& $bash -lc "cd /$letter && ./tools/cl.sh $Command $argString"
exit $LASTEXITCODE
