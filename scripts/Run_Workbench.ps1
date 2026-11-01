# Build and run Unified Engineering Workbench on Windows without touching ./build.
$ErrorActionPreference = 'Stop'
$ProjectRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
Set-Location $ProjectRoot

function Fail([string]$Message) {
    Write-Host "`nERROR: $Message" -ForegroundColor Red
    Read-Host 'Press Enter to close'
    exit 1
}

$BuildDir = Join-Path $ProjectRoot 'build-windows'
$Generator = $null
$Compiler = $null
$UseMSVC = $false

# Prefer Visual Studio 2022 x64 when installed; otherwise use MSYS2 UCRT64.
$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
if (Test-Path $vswhere) {
    $vs = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
    if ($vs) { $Generator = 'Visual Studio 17 2022'; $UseMSVC = $true }
}

if (-not $UseMSVC) {
    $ucrtBin = 'C:\msys64\ucrt64\bin'
    $ucrtGxx = Join-Path $ucrtBin 'g++.exe'
    if (-not (Test-Path $ucrtGxx)) {
        Fail 'No Visual Studio 2022 C++ workload or MSYS2 UCRT64 compiler was found. Install Visual Studio 2022 Desktop development with C++, or MSYS2 UCRT64 GCC + Ninja. The old C:\MinGW GCC 6.3 is not supported.'
    }
    $env:Path = "$ucrtBin;C:\msys64\usr\bin;$env:Path"
    $Compiler = $ucrtGxx
    $Generator = 'Ninja'
    $version = (& $Compiler --version | Select-Object -First 1)
    Write-Host "Using MSYS2 compiler: $version" -ForegroundColor Cyan
    if ($version -match '6\.3\.0') { Fail 'Detected obsolete GCC 6.3. Use the MSYS2 UCRT64 GCC compiler.' }
}

if (-not (Get-Command cmake -ErrorAction SilentlyContinue)) { Fail 'CMake is not installed or is not on PATH.' }

if (Test-Path $BuildDir) {
    $stamp = Get-Date -Format 'yyyyMMdd-HHmmss'
    $BuildDir = Join-Path $ProjectRoot "build-windows-$stamp"
}

if ($UseMSVC) {
    Write-Host 'Configuring with Visual Studio 2022 x64...' -ForegroundColor Cyan
    cmake -S . -B $BuildDir -G $Generator -A x64
} else {
    if (-not (Get-Command ninja -ErrorAction SilentlyContinue)) { Fail 'Ninja is required for the MSYS2 UCRT64 build. Install it with: pacman -S mingw-w64-ucrt-x86_64-ninja' }
    Write-Host 'Configuring with MSYS2 UCRT64 + Ninja...' -ForegroundColor Cyan
    cmake -S . -B $BuildDir -G Ninja -DCMAKE_CXX_COMPILER="$Compiler" -DCMAKE_BUILD_TYPE=Release
}
if ($LASTEXITCODE -ne 0) { Fail 'CMake configuration failed. Read the first CMake error above.' }

Write-Host 'Building server and CLI...' -ForegroundColor Cyan
if ($UseMSVC) { cmake --build $BuildDir --config Release --target uew_server workbench workbench_tests } else { cmake --build $BuildDir --target uew_server workbench workbench_tests --parallel }
if ($LASTEXITCODE -ne 0) { Fail 'Compilation failed. The build log above contains the compiler error.' }

Write-Host 'Running automated tests...' -ForegroundColor Cyan
if ($UseMSVC) { ctest --test-dir $BuildDir -C Release --output-on-failure } else { ctest --test-dir $BuildDir --output-on-failure }
if ($LASTEXITCODE -ne 0) { Fail 'Automated tests failed; server will not be started.' }

$Server = if ($UseMSVC) { Join-Path $BuildDir 'Release\uew_server.exe' } else { Join-Path $BuildDir 'uew_server.exe' }
if (-not (Test-Path $Server)) { Fail "Build completed but server executable was not found: $Server" }

# The server uses relative frontend/, data/ and results/ paths. Always launch with project root as cwd.
$env:UEW_PORT = '8080'
Write-Host "`nStarting C++ backend: $Server" -ForegroundColor Green
Write-Host 'Keep this window open while using the app. Press Ctrl+C to stop it.' -ForegroundColor Yellow
Write-Host 'Open this address in Chrome: http://127.0.0.1:8080/' -ForegroundColor Cyan
& $Server
if ($LASTEXITCODE -ne 0) { Fail "Server exited with code $LASTEXITCODE. Check whether port 8080 is already in use or inspect the runtime error above." }
