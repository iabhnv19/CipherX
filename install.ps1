# CipherX one-line installer
$ErrorActionPreference = "Stop"

[Net.ServicePointManager]::SecurityProtocol =
    [Net.ServicePointManager]::SecurityProtocol -bor [Net.SecurityProtocolType]::Tls12

$installDir = Join-Path $env:LOCALAPPDATA "CipherX"
$binPath = Join-Path $installDir "project.exe"
$shimCmd = Join-Path $installDir "cipherx.cmd"

Write-Host ""
Write-Host "  CipherX - Offline Crypto Shell" -ForegroundColor Cyan
Write-Host "  Installing to $installDir..." -ForegroundColor DarkGray

# ensure compiler exists
$gpp = Get-Command g++ -ErrorAction SilentlyContinue
if (-not $gpp) {
    Write-Host ""
    Write-Host "  [!] g++ compiler not found on this system." -ForegroundColor Yellow
    Write-Host "  Run this to install/configure GCC automatically:" -ForegroundColor Cyan
    Write-Host "  irm https://raw.githubusercontent.com/iabhnv19/gcc-auto-setup/main/install.ps1 | iex" -ForegroundColor White
    Write-Host ""
    exit 1
}

New-Item -ItemType Directory -Path $installDir -Force | Out-Null

$tempDir = Join-Path $env:TEMP ("cipherx_build_" + [Guid]::NewGuid().ToString("N"))
New-Item -ItemType Directory -Path $tempDir -Force | Out-Null

try {
    Write-Host "  Fetching latest sources..." -ForegroundColor DarkGray
    $repoUrl = "https://raw.githubusercontent.com/iabhnv19/CipherX/main"
    $headers = @("actions.h", "bluetooth.h", "ciphers.h", "common.h", "cryptanalysis.h", "vault.h")
    $sources = @("actions.cpp", "bluetooth.cpp", "ciphers.cpp", "common.cpp", "cryptanalysis.cpp", "main.cpp", "vault.cpp")

    $incDir = Join-Path $tempDir "include"
    $srcDir = Join-Path $tempDir "src"
    New-Item -ItemType Directory -Path $incDir -Force | Out-Null
    New-Item -ItemType Directory -Path $srcDir -Force | Out-Null

    foreach ($h in $headers) {
        Invoke-WebRequest -Uri "$repoUrl/include/$h" -OutFile (Join-Path $incDir $h) -UseBasicParsing
    }
    foreach ($s in $sources) {
        Invoke-WebRequest -Uri "$repoUrl/src/$s" -OutFile (Join-Path $srcDir $s) -UseBasicParsing
    }

    Write-Host "  Compiling with g++..." -ForegroundColor DarkGray
    $srcFiles = (Get-ChildItem -Path $srcDir -Filter *.cpp).FullName
    & g++ -std=c++14 -O2 -I"$incDir" $srcFiles -o "$binPath" -lws2_32 -lbthprops

    if (-not (Test-Path $binPath)) {
        throw "Compilation finished but binary was not produced."
    }

    # create launcher shim so 'cipherx' runs in any terminal
    $shim = "@\"$binPath\" %*"
    Set-Content -Path $shimCmd -Value $shim -Encoding Ascii

    # add to User PATH
    $userPath = [Environment]::GetEnvironmentVariable("Path", "User")
    $entries  = @(($userPath -split ";") | Where-Object { $_.Trim() -ne "" })
    if ($entries -notcontains $installDir) {
        [Environment]::SetEnvironmentVariable("Path", (($entries + $installDir) -join ";"), "User")
    }
    if (($env:Path -split ";") -notcontains $installDir) { $env:Path += ";$installDir" }

    Write-Host ""
    Write-Host "  Installed successfully." -ForegroundColor Green
    Write-Host "  Type 'cipherx' in any terminal to launch." -ForegroundColor Cyan
    Write-Host ""
}
finally {
    Remove-Item $tempDir -Recurse -Force -ErrorAction SilentlyContinue
}

