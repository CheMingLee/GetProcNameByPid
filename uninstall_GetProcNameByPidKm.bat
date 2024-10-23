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

echo Deleting the service
sc delete GetProcNameByPidKm

echo Uninstalling the driver
for /f "tokens=1,2*" %%A in ('pnputil /enum-drivers ^| findstr /C:"Original Name" ^| findstr /C:"getprocnamebypidkm.inf"') do (
    echo Original Name: %%C

    for /f "tokens=3" %%X in ('pnputil /enum-drivers ^| findstr /C:"Published Name"') do (
        echo oem#.inf: %%X
        pnputil /delete-driver %%X /uninstall
        if %errorLevel% NEQ 0 (
            echo Failed to uninstall the driver
            pause
            exit /b 1
        )
        echo Uninstalled %%X
        goto end
    )
)
if %errorLevel% NEQ 0 (
    echo Failed to enum the drivers
    pause
    exit /b 1
)

:end
echo Done

endlocal
pause