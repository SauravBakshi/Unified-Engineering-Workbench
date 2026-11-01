$ErrorActionPreference = 'Stop'
$ProjectRoot = Split-Path -Parent $PSScriptRoot
$BuildDir = Join-Path $ProjectRoot 'build-win-x64'
$PackageDir = Join-Path $ProjectRoot 'dist\CandidateID_Workbench'

$Gxx = Get-Command g++.exe -ErrorAction SilentlyContinue
if ($Gxx) {
  $Ninja = Get-Command ninja.exe -ErrorAction SilentlyContinue
  if ($Ninja) {
    cmake -S $ProjectRoot -B $BuildDir -G Ninja "-DCMAKE_CXX_COMPILER=$($Gxx.Source)" -DCMAKE_BUILD_TYPE=Release -DUEW_BUILD_TESTS=ON
  } else {
    $Make = Get-Command mingw32-make.exe -ErrorAction SilentlyContinue
    if (!$Make) { $Make = Get-Command make.exe -ErrorAction SilentlyContinue }
    if (!$Make) { throw 'MinGW g++ found, but neither Ninja nor mingw32-make is available. Add one build tool to PATH.' }
    cmake -S $ProjectRoot -B $BuildDir -G 'MinGW Makefiles' "-DCMAKE_MAKE_PROGRAM=$($Make.Source)" "-DCMAKE_CXX_COMPILER=$($Gxx.Source)" -DCMAKE_BUILD_TYPE=Release -DUEW_BUILD_TESTS=ON
  }
} else {
  cmake -S $ProjectRoot -B $BuildDir -G 'Visual Studio 17 2022' -A x64 -DUEW_BUILD_TESTS=ON
}
if ($LASTEXITCODE -ne 0) { throw 'CMake configure failed.' }
cmake --build $BuildDir --config Release --parallel
if ($LASTEXITCODE -ne 0) { throw 'Windows x64 build failed.' }
ctest --test-dir $BuildDir -C Release --output-on-failure
if ($LASTEXITCODE -ne 0) { throw 'Windows CTest suite failed.' }

if (Test-Path $PackageDir) { Remove-Item $PackageDir -Recurse -Force }
New-Item -ItemType Directory -Path $PackageDir | Out-Null
$BinDir = if (Test-Path (Join-Path $BuildDir 'Release\workbench.exe')) { Join-Path $BuildDir 'Release' } else { $BuildDir }
Push-Location $ProjectRoot
try {
  & (Join-Path $BinDir 'workbench.exe') demo
  if ($LASTEXITCODE -ne 0) { throw 'Target-machine demo/result generation failed.' }
  if (Get-Command python -ErrorAction SilentlyContinue) {
    python (Join-Path $PSScriptRoot 'generate_plots.py')
    if ($LASTEXITCODE -ne 0) { Write-Warning 'Plot regeneration skipped; install Matplotlib.' }
    python (Join-Path $PSScriptRoot 'build_report_pdf.py')
    if ($LASTEXITCODE -ne 0) { Write-Warning 'PDF report regeneration skipped; install ReportLab.' }
  }
} finally { Pop-Location }
foreach ($exe in @('Launch.exe','uew_server.exe','workbench.exe','workbench_tests.exe')) {
  $source = Join-Path $BinDir $exe
  if (!(Test-Path $source)) { throw "Required build output missing: $source" }
  Copy-Item $source $PackageDir
}
foreach ($dir in @('frontend','data','docs','results','tests','include','src','third_party','.vscode','config','scripts')) {
  Copy-Item (Join-Path $ProjectRoot $dir) $PackageDir -Recurse
}
foreach ($file in @('README.md','CMakeLists.txt','.gitignore')) { Copy-Item (Join-Path $ProjectRoot $file) $PackageDir }
if (Test-Path (Join-Path $ProjectRoot 'version_control')) { Copy-Item (Join-Path $ProjectRoot 'version_control') $PackageDir -Recurse }
Write-Host "Package created at $PackageDir"
Write-Host 'Before submission: test Launch.exe after extracting this folder on a clean Windows 11 x64 machine.'
