<#
.SYNOPSIS
    Recompila el ELF del juego a C++ y lo copia, con los overrides, al runner de PS2Recomp.

.DESCRIPTION
    Flujo de la base de Waffle565 (vía KIexster/god-of-war-recomp, ver docs/KIEXSTER.md):
    1. Rellena config/recomp.template.toml (@ELF@, @MAP@, @OUT@) -> out/gow.effective.toml
       y ejecuta ps2_recomp con él -> generated/
    2. IOP nativo (opcional): si $env:GOW_IOP_MODULOS apunta a una carpeta con modules.txt (sale de una
       ejecución con $env:PS2X_IOP_DUMP_MODULES), tools/iop/iopn_gen.py compila los IRX a C++ dentro de
       PS2Recomp/ps2xIOP/src/native (ignorado por git: son datos derivados del juego).
    3. Copia generated/* + overrides/*.cpp + overrides/*.h a PS2Recomp/ps2xRuntime/src/runner/
    4. Reconfigura CMake. VU1 nativa (opcional): si $env:GOW_VU1_MICROCODIGO apunta a una carpeta con
       micromemorias capturadas con $env:GOW_VU1_CAPTURA, CMake genera y compila la VU1 nativa.

.PARAMETER SyncOnly
    No recompila: solo vuelve a copiar generated/ y overrides/ al runner (útil tras editar un override).

.PARAMETER AggressiveLogs
    Reconfigura CMake con AGRESSIVE_LOGS activado (por defecto, desactivado; ver build-tools.ps1).
    Cambiarlo obliga a una compilación completa.
#>
param(
    [switch]$SyncOnly,
    [switch]$AggressiveLogs
)
. "$PSScriptRoot\_common.ps1"

$elfPath = Get-GameElfPath
function ConvertTo-ForwardSlash([string]$Path) { return $Path -replace '\\', '/' }

if (-not $SyncOnly) {
    $recomp = Join-Path $BuildDir 'ps2xRecomp\Release\ps2_recomp.exe'
    if (-not (Test-Path $recomp)) { throw 'Falta ps2_recomp.exe. Ejecuta primero .\scripts\build-tools.ps1' }

    # ps2_recomp no abre rutas con caracteres no ASCII (acentos en la carpeta del repo, por ejemplo)
    if ("$RepoRoot" -match '[^\x00-\x7F]') { throw "La ruta del repo tiene caracteres no ASCII: $RepoRoot" }

    $toml = [IO.File]::ReadAllText($TemplateToml)
    $toml = $toml.Replace('@ELF@', (ConvertTo-ForwardSlash $elfPath)).
                  Replace('@MAP@', (ConvertTo-ForwardSlash $FuncMapCsv)).
                  Replace('@OUT@', (ConvertTo-ForwardSlash $GeneratedDir) + '/')
    New-Item -ItemType Directory -Force (Split-Path $EffectiveToml) | Out-Null
    Write-Utf8NoBom $EffectiveToml $toml

    if (Test-Path $GeneratedDir) { Remove-Item $GeneratedDir -Recurse -Force }
    New-Item -ItemType Directory -Force $GeneratedDir | Out-Null

    Push-Location $RepoRoot
    try { Invoke-Native $recomp @($EffectiveToml) }
    finally { Pop-Location }
}

# ── IOP nativo (opcional) ──────────────────────────────────────────────────
$iopNativeDir = Join-Path $RecompDir 'ps2xIOP\src\native'
if ($env:GOW_IOP_MODULOS) {
    if (-not (Test-Path (Join-Path $env:GOW_IOP_MODULOS 'modules.txt'))) { throw "GOW_IOP_MODULOS no tiene modules.txt: $env:GOW_IOP_MODULOS" }
    Write-Host "IOP nativo desde $env:GOW_IOP_MODULOS"
    # Se genera aparte y solo se copia lo que cambia: si no, cada -SyncOnly deja fecha nueva a los iopn_*.cpp y MSBuild
    # recompila el IOP (el de 989NOMID son 10 MB en un solo cl.exe, varios minutos).
    $iopTemp = Join-Path $RepoRoot 'out\iopn-gen'
    if (Test-Path $iopTemp) { Remove-Item $iopTemp -Recurse -Force }
    New-Item -ItemType Directory -Force $iopTemp | Out-Null
    Invoke-Native 'py' @('-3', '-I', (Join-Path $RepoRoot 'tools\iop\iopn_gen.py'), $env:GOW_IOP_MODULOS, $iopTemp)
    $fresh = @(Get-ChildItem $iopTemp -Filter 'iopn_*.cpp' -File)
    $freshNames = $fresh | ForEach-Object Name
    Get-ChildItem $iopNativeDir -Filter 'iopn_*.cpp' -File -ErrorAction SilentlyContinue |
        Where-Object { $freshNames -notcontains $_.Name } | Remove-Item -Force
    $iopCopied = 0
    foreach ($file in $fresh) {
        $dest = Join-Path $iopNativeDir $file.Name
        if ((Test-Path $dest) -and (Get-FileHash $dest -Algorithm MD5).Hash -eq (Get-FileHash $file.FullName -Algorithm MD5).Hash) { continue }
        Copy-Item $file.FullName $dest -Force
        $iopCopied++
    }
    Write-Host "IOP nativo: $($fresh.Count) archivos, $iopCopied copiados (contenido nuevo o distinto)."
} else {
    Get-ChildItem $iopNativeDir -Filter 'iopn_*.cpp' -File -ErrorAction SilentlyContinue | Remove-Item -Force
}

# ── Sincronizar con el runner ──────────────────────────────────────────────
if (-not (Test-Path $GeneratedDir)) { throw 'No hay nada en generated/. Ejecuta sin -SyncOnly.' }

New-Item -ItemType Directory -Force (Split-Path $SyncManifest) | Out-Null

# Solo se copian los archivos cuyo contenido cambió: los demás conservan su fecha y MSBuild no los recompila.
# Antes se borraba y se copiaba todo con la fecha actual, y cada sincronización recompilaba el runner entero
# (~45 min) aunque nada hubiera cambiado. Primero se compara el tamaño y, si coincide, una huella MD5.
$md5 = [System.Security.Cryptography.MD5]::Create()
function Test-SameContent([string]$a, [string]$b) {
    $fa = Get-Item -LiteralPath $a
    $fb = Get-Item -LiteralPath $b -ErrorAction SilentlyContinue
    if (-not $fb -or $fa.Length -ne $fb.Length) { return $false }
    $ha = $md5.ComputeHash([IO.File]::ReadAllBytes($a))
    $hb = $md5.ComputeHash([IO.File]::ReadAllBytes($b))
    return [Convert]::ToBase64String($ha) -eq [Convert]::ToBase64String($hb)
}

# El código generado incluye <ps2_recompiled_functions.h> desde src/runner, así que todo va ahí
$copied = New-Object System.Collections.Generic.List[string]
$changed = 0
$sources = @(Get-ChildItem $GeneratedDir -File) + @(Get-ChildItem $OverridesDir -File | Where-Object { $_.Extension -in '.cpp', '.h' })
foreach ($file in $sources) {
    $dest = Join-Path $RunnerSrcDir $file.Name
    if (-not (Test-SameContent $file.FullName $dest)) {
        Copy-Item $file.FullName $dest -Force
        (Get-Item $dest).LastWriteTime = Get-Date   # que MSBuild no lo dé por compilado
        $changed++
    }
    $copied.Add($dest)
}
# Borrar las copias de la vez anterior que ya no existen (funciones renombradas o eliminadas)
$removed = 0
if (Test-Path $SyncManifest) {
    $current = New-Object 'System.Collections.Generic.HashSet[string]' ([StringComparer]::OrdinalIgnoreCase)
    foreach ($path in $copied) { [void]$current.Add($path) }
    foreach ($old in (Get-Content $SyncManifest)) {
        if ($old -and -not $current.Contains($old) -and (Test-Path -LiteralPath $old)) {
            Remove-Item -LiteralPath $old -Force
            $removed++
        }
    }
}
Set-Content -Path $SyncManifest -Value $copied -Encoding utf8
Write-Host "Runner de PS2Recomp: $($copied.Count) archivos, $changed copiados (contenido nuevo o distinto), $removed borrados."

# Reconfigurar CMake para que el proyecto del runner incluya los archivos nuevos. Si se deja
# a la regeneración automática durante el build, MSBuild compila con el proyecto viejo y
# el enlazado falla con miles de LNK2019.
$cmakeArgs = @('-S', $RecompDir, '-B', $BuildDir, (Get-AggressiveLogsDefine $AggressiveLogs))
$vu1Micro = if ($env:GOW_VU1_MICROCODIGO) { ConvertTo-ForwardSlash $env:GOW_VU1_MICROCODIGO } else { '' }
if ($vu1Micro) { Write-Host "VU1 nativa desde $vu1Micro" }
$cmakeArgs += "-DPS2X_VU1_MICROCODE_DIR=$vu1Micro"
$cmakeArgs += "-DPS2X_VU1_GENERATOR_SOURCE=$(if ($vu1Micro) { ConvertTo-ForwardSlash (Join-Path $RepoRoot 'tools\vu1\generar_vu1.cpp') })"
$cmakeArgs += "-DPS2X_VU1N_GEN_OVERRIDE=$(if ($vu1Micro) { ConvertTo-ForwardSlash (Join-Path $RepoRoot 'tools\vu1\vu1n_gen.py') })"
$cmakeArgs += "-DPS2X_VU1N_HEADS_FILE=$(if ($vu1Micro) { ConvertTo-ForwardSlash (Join-Path $RepoRoot 'tools\vu1\vu1n_heads.txt') })"
if (Test-Path (Join-Path $BuildDir 'CMakeCache.txt')) {
    Invoke-Native (Get-CMake) $cmakeArgs | Out-Null
}
Write-Host 'Siguiente paso: .\scripts\run.ps1 -Config RelWithDebInfo'
