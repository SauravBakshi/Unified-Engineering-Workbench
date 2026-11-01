@echo off
setlocal
cd /d "%~dp0.."
if not exist "Launch.exe" (
  echo Launch.exe is missing. Run scripts\package_windows.ps1 on Windows to create the standalone package.
  exit /b 1
)
start "" "Launch.exe"
