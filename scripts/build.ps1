<#
    build.ps1 - Compila il firmware per la Longer LK4 Pro
    Uso:  powershell -ExecutionPolicy Bypass -File scripts\build.ps1 [stock|plus|silentE]
#>
param(
    [ValidateSet('stock', 'plus', 'silentE')]
    [string]$Variant = 'stock'
)

$ErrorActionPreference = 'Stop'
$Repo = Split-Path -Parent $PSScriptRoot
Set-Location $Repo

$envMap = @{
    'stock'   = 'LK4Pro'
    'plus'    = 'LK4Pro_Plus'
    'silentE' = 'LK4Pro_SilentE'
}
$env = $envMap[$Variant]

$pio = Get-Command pio.exe -ErrorAction SilentlyContinue
if (-not $pio) {
    $fallback = "$env:APPDATA\Python\Python313\Scripts\pio.exe"
    if (Test-Path $fallback) { $pio = @{ Source = $fallback } }
    else { Write-Host "Serve PlatformIO: pip install platformio" -ForegroundColor Red; exit 1 }
}

Write-Host "Build di $env (variante $Variant)..." -ForegroundColor Cyan
& $pio.Source run -e $env
Write-Host ""
Write-Host "Firmware in .pio\firmware\ :" -ForegroundColor Cyan
Get-ChildItem .pio\firmware -File | Select-Object Name, Length, LastWriteTime
