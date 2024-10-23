@echo off
setlocal

:: Check for administrative privileges
net session >nul 2>&1
if %errorLevel% NEQ 0 (
    echo This script requires administrative privileges.
    pause
    exit /b 1
)

cd /d %~dp0
echo Running the script from %cd%

set DRIVER_DIR=%cd%\GetProcNameByPidKm
if not exist "%DRIVER_DIR%" (
    echo %DRIVER_DIR% not found
    pause
    exit /b 1
)

echo Installing the driver from %DRIVER_DIR%
pnputil /add-driver "%DRIVER_DIR%\GetProcNameByPidKm.inf" /install
if %errorLevel% NEQ 0 (
    echo Failed to install the driver
    pause
    exit /b 1
)

echo Starting the service
sc start GetProcNameByPidKm
if %errorLevel% NEQ 0 (
    echo Failed to start the service
    pause
    exit /b 1
)

echo Done

endlocal
pause