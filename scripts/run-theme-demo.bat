@echo off
setlocal

set "SCRIPT_DIR=%~dp0"
set "PS_SCRIPT=%SCRIPT_DIR%run-theme-demo.ps1"

if not exist "%PS_SCRIPT%" (
  echo [run-theme-demo] script not found: "%PS_SCRIPT%"
  exit /b 1
)

set "POWERSHELL_EXE=%SystemRoot%\System32\WindowsPowerShell\v1.0\powershell.exe"
if exist "%POWERSHELL_EXE%" (
  "%POWERSHELL_EXE%" -NoProfile -ExecutionPolicy Bypass -File "%PS_SCRIPT%" %*
) else (
  pwsh -NoProfile -ExecutionPolicy Bypass -File "%PS_SCRIPT%" %*
)
set "EXIT_CODE=%ERRORLEVEL%"

if not "%EXIT_CODE%"=="0" (
  echo [run-theme-demo] failed with exit code %EXIT_CODE%
)

exit /b %EXIT_CODE%
