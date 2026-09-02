#Requires -Version 5.1
param(
    [Parameter(Position = 0)]
    [string]$Que = "app"
)

$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $MyInvocation.MyCommand.Path
$build = Join-Path $root "build"
$vsdev = "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\Tools\VsDevCmd.bat"

$alias = @{
    app        = @("MiguelMusicAssistant_Standalone")
    vst3       = @("MiguelMusicAssistant_VST3")
    beta       = @("MiguelMusicAssistant_Standalone", "MiguelMusicAssistant_VST3")
    hp         = @("BajxterFxHp")
    lp         = @("BajxterFxLp")
    compresor  = @("BajxterFxComp")
    excitador  = @("BajxterFxExciter")
    duplicador = @("BajxterFxDoubler")
    distorsion = @("BajxterFxDist")
    delay      = @("BajxterFxDelay")
    efecto     = @("BajxterFxEfecto")
    volumen    = @("BajxterFxVolume")
    velocity   = @("BajxterFxVelocity")
    afinador   = @("BajxterTuner")
    eq         = @("BajxterEq")
    fx         = @(
        "BajxterFxHp", "BajxterFxLp", "BajxterFxComp", "BajxterFxExciter",
        "BajxterFxDoubler", "BajxterFxDist", "BajxterFxDelay", "BajxterFxEfecto",
        "BajxterFxVolume", "BajxterFxVelocity", "BajxterTuner", "BajxterEq"
    )
}

$key = $Que.ToLowerInvariant()
if (-not $alias.ContainsKey($key)) {
    Write-Error "Usa: app, vst3, beta, fx, delay, eq, afinador, compresor, ..."
}

$targets = $alias[$key]
$targetFlags = ($targets | ForEach-Object { "--target $_" }) -join " "
$cmd = @"
call "$vsdev" -arch=amd64 && cmake -S "$root" -B "$build" && cmake --build "$build" --config Release --parallel 4 $targetFlags
"@
Write-Host "Compilando: $($targets -join ', ')"
cmd /c $cmd
exit $LASTEXITCODE
