#Requires -Version 5.1
# Copia PICHADAW (app ya instalada + VST3) a las carpetas de uso.
# No compila. No pisa Escritorio\PICHADAW-conservado\PICHADAW.exe

$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $MyInvocation.MyCommand.Path
$desk2 = "C:\Users\MIGUEL\OneDrive2\Desktop"
$desk = if (Test-Path -LiteralPath $desk2) {
    $desk2
} else {
    [Environment]::GetFolderPath("Desktop")
}
$progDir = Join-Path $env:LOCALAPPDATA "Programs\Miguel Music Assistant"
$conservado = Join-Path $desk "PICHADAW-conservado"
$txtSrc = Join-Path $root "PICHADAW.txt"
$icoSrc = Join-Path $root "Assets\PICHADAW.ico"

function Copy-Vst3Bundle([string]$from, [string]$to) {
    $fromFull = [IO.Path]::GetFullPath($from)
    $toFull = [IO.Path]::GetFullPath($to)
    if ($fromFull -eq $toFull) {
        return
    }
    $parent = Split-Path -Parent $to
    New-Item -ItemType Directory -Force -Path $parent | Out-Null
    if (Test-Path -LiteralPath $to) {
        Remove-Item -LiteralPath $to -Recurse -Force
    }
    Copy-Item -LiteralPath $from -Destination $to -Recurse -Force
}

New-Item -ItemType Directory -Force -Path $progDir | Out-Null
New-Item -ItemType Directory -Force -Path $conservado | Out-Null

$progExe = Join-Path $progDir "PICHADAW.exe"
$keepExe = Join-Path $conservado "PICHADAW.exe"
if (-not (Test-Path -LiteralPath $progExe) -and (Test-Path -LiteralPath $keepExe)) {
    Copy-Item -LiteralPath $keepExe -Destination $progExe -Force
    Write-Host "Restaurado exe de conservado a Programas"
}

if (Test-Path -LiteralPath $icoSrc) {
    Copy-Item -LiteralPath $icoSrc -Destination (Join-Path $progDir "PICHADAW.ico") -Force
}

$sh = New-Object -ComObject WScript.Shell
$lnkPath = Join-Path $desk "PICHADAW.lnk"
$lnk = $sh.CreateShortcut($lnkPath)
$lnk.TargetPath = $progExe
$lnk.WorkingDirectory = $progDir
$lnk.Description = "PICHADAW"
$icoDst = Join-Path $progDir "PICHADAW.ico"
if (Test-Path -LiteralPath $icoDst) {
    $lnk.IconLocation = "$icoDst,0"
}
$lnk.Save()
Write-Host "Acceso: $lnkPath -> $progExe"

$vstSrc = Join-Path $root "build\MiguelMusicAssistant_artefacts\Release\VST3\PICHADAW.vst3"
if (-not (Test-Path -LiteralPath $vstSrc)) {
    $vstSrc = Join-Path $env:USERPROFILE "Documents\Miguel Music Assistant\VST3\PICHADAW.vst3"
}
if (-not (Test-Path -LiteralPath $vstSrc)) {
    $vstSrc = "C:\Program Files\Common Files\VST3\PICHADAW.vst3"
}
if (-not (Test-Path -LiteralPath $vstSrc)) {
    Write-Error "No encontre PICHADAW.vst3. Compila antes: .\compilar.ps1 vst3"
}

$userVst = Join-Path $env:USERPROFILE "Documents\Miguel Music Assistant\VST3\PICHADAW.vst3"
Copy-Vst3Bundle $vstSrc $userVst
Write-Host "VST3 usuario: $userVst"

$sysVst = "C:\Program Files\Common Files\VST3\PICHADAW.vst3"
try {
    Copy-Vst3Bundle $vstSrc $sysVst
    Write-Host "VST3 FL: $sysVst"
}
catch {
    Write-Host "No pude copiar a Common Files (admin). FL puede usar: $userVst"
}

if (Test-Path -LiteralPath $txtSrc) {
    Copy-Item -LiteralPath $txtSrc -Destination (Join-Path $desk "PICHADAW.txt") -Force
    Copy-Item -LiteralPath $txtSrc -Destination (Join-Path $conservado "PICHADAW.txt") -Force
    Copy-Item -LiteralPath $txtSrc -Destination (Join-Path $progDir "PICHADAW.txt") -Force
    Write-Host "Texto copiado al Escritorio, conservado y Programas"
}

Write-Host "Listo. Deja PICHADAW.lnk. No borres PICHADAW-conservado."
exit 0
