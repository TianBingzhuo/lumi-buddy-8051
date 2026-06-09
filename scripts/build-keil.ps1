$ErrorActionPreference = "Stop"

$Root = Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)
$Build = Join-Path $Root "build"
$Src = Join-Path $Root "src\main.c"
$C51 = "C:\Keil_v5\C51\BIN\C51.exe"
$BL51 = "C:\Keil_v5\C51\BIN\BL51.exe"
$OH51 = "C:\Keil_v5\C51\BIN\OH51.exe"

New-Item -ItemType Directory -Force -Path $Build | Out-Null
Copy-Item -LiteralPath $Src -Destination (Join-Path $Build "main.c") -Force

Push-Location $Build
try {
    & $C51 @("main.c", "OBJECT(main.obj)", "PRINT(main.lst)", "OPTIMIZE(8)", "BROWSE", "DEBUG")
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

    & $BL51 @("MAIN.OBJ", "TO", "LUMI_EXP08_RESERVED", "PRINT(lumi_exp08_reserved.m51)")
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

    & $OH51 @("LUMI_EXP08_RESERVED", "HEXFILE(lumi_exp08_reserved.hex)")
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}
finally {
    Pop-Location
}

Write-Host "Built: $Build\lumi_exp08_reserved.hex"
