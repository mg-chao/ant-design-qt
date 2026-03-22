@echo off
setlocal

set "SCRIPT_DIR=%~dp0"
set "ENTRY_SCRIPT=%SCRIPT_DIR%run-theme-demo.bat"

if not exist "%ENTRY_SCRIPT%" (
  echo [run-theme-demo-release] script not found: "%ENTRY_SCRIPT%"
  exit /b 1
)

call "%ENTRY_SCRIPT%" -Config Release %*
exit /b %ERRORLEVEL%
