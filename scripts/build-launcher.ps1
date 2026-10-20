<#
.SYNOPSIS
    Builds the port's launcher (launcher/, C++ with raylib + Dear ImGui) and optionally starts it.

.DESCRIPTION
    Configures launcher/ into out\launcher-build and builds GoW2Launcher.exe. It uses the same raylib, Dear ImGui and
    rlImGui versions as PS2Recomp's runtime; if the runner build already downloaded them (out\build\_deps), CMake reuses
    those copies instead of fetching them again. The launcher is front end only for now: its settings are not saved
    or applied yet.

.PARAMETER Config
    Build configuration (Release by default).

.PARAMETER Run
    Start the launcher after building it.

.PARAMETER Generator
    CMake generator. VS 2022 by default.
#>
param(
    [string]$Config = 'Release',
    [switch]$Run,
    [string]$Generator = 'Visual Studio 17 2022'
)
. "$PSScriptRoot\_common.ps1"

$cmake = Get-CMake
$source = Join-Path $RepoRoot 'launcher'
$build = Join-Path $RepoRoot 'out\launcher-build'

$configure = @('-S', $source, '-B', $build, '-G', $Generator, '-A', 'x64')
# reuse what the runner build already downloaded
$deps = Join-Path $BuildDir '_deps'
foreach ($pair in @(@('RAYLIB', 'raylib-src'), @('IMGUI', 'imgui-src'), @('RLIMGUI', 'rlimgui-src'))) {
    $dir = Join-Path $deps $pair[1]
    if (Test-Path (Join-Path $dir '*')) {
        $configure += "-DFETCHCONTENT_SOURCE_DIR_$($pair[0])=$($dir -replace '\\', '/')"
    }
}

Invoke-Native $cmake $configure
Invoke-Native $cmake @('--build', $build, '--config', $Config, '--target', 'GoW2Launcher', '--parallel')

$exe = Join-Path $build "$Config\GoW2Launcher.exe"
Write-Host ''
Write-Host "Launcher ready: $exe"
if ($Run) {
    Start-Process -FilePath $exe -WorkingDirectory (Split-Path $exe)
}
