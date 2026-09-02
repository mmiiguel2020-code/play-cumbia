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
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

if ($key -eq "app" -or $key -eq "beta") {
    $exe = Join-Path $build "MiguelMusicAssistant_artefacts\Release\Standalone\PICHADAW.exe"
    if (-not (Test-Path -LiteralPath $exe)) {
        Write-Error "No salio PICHADAW.exe"
    }
    $progDir = Join-Path $env:LOCALAPPDATA "Programs\Miguel Music Assistant"
    $desk = [Environment]::GetFolderPath("Desktop")
    New-Item -ItemType Directory -Force -Path $progDir | Out-Null
    Copy-Item -LiteralPath $exe -Destination (Join-Path $progDir "PICHADAW.exe") -Force
    Copy-Item -LiteralPath $exe -Destination (Join-Path $desk "PICHADAW.exe") -Force
    $icoSrc = Join-Path $root "Assets\PICHADAW.ico"
    $icoDst = Join-Path $progDir "PICHADAW.ico"
    if (Test-Path -LiteralPath $icoSrc) {
        Copy-Item -LiteralPath $icoSrc -Destination $icoDst -Force
    }
    $sh = New-Object -ComObject WScript.Shell
    $lnk = $sh.CreateShortcut((Join-Path $desk "PICHADAW.lnk"))
    $lnk.TargetPath = Join-Path $progDir "PICHADAW.exe"
    $lnk.WorkingDirectory = $progDir
    $lnk.Description = "PICHADAW"
    if (Test-Path -LiteralPath $icoDst) {
        $lnk.IconLocation = "$icoDst,0"
    }
    $lnk.Save()
    foreach ($old in @(
            (Join-Path $desk "Bajxterbeta.exe"),
            (Join-Path $desk "Bajxterbeta.lnk"),
            (Join-Path $progDir "Bajxterbeta.exe"),
            (Join-Path $progDir "Bajxter Producer.exe"),
            (Join-Path $progDir "Miguel Music Assistant.exe")
        )) {
        if (Test-Path -LiteralPath $old) {
            Remove-Item -LiteralPath $old -Force
            Write-Host "Quitado $old"
        }
    }
    Write-Host "Instalado PICHADAW.exe (Escritorio y Programas) y PICHADAW.lnk"
}

exit 0
