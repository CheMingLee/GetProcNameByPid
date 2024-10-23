@echo off
setlocal

:: Check for administrative privileges
net session >nul 2>&1
if %errorLevel% NEQ 0 (
    echo This script requires administrative privileges.
    pause
    exit /b 1
)

echo Stopping the service
sc stop GetProcNameByPidKm
if %errorLevel% NEQ 0 (
    echo Failed to stop the service
    pause
    exit /b 1
)

echo Done

endlocal
pause