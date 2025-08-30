@echo off
REM Windows Service Installation Script for ssftpd
REM Requires NSSM (Non-Sucking Service Manager) to be installed

setlocal enabledelayedexpansion

REM Check if running as Administrator
net session >nul 2>&1
if %errorLevel% neq 0 (
    echo ERROR: This script must be run as Administrator
    echo Right-click and select "Run as Administrator"
    pause
    exit /b 1
)

REM Set variables
set SERVICE_NAME=ssftpd
set SERVICE_DISPLAY=Simple-Secure FTP Daemon
set EXECUTABLE_PATH=C:\Program Files\ssftpd\bin\ssftpd.exe
set CONFIG_PATH=C:\Program Files\ssftpd\etc\deployment\ssftpd.conf
set WORKING_DIR=C:\Program Files\ssftpd
set LOG_DIR=C:\Program Files\ssftpd\logs

REM Check if NSSM is available
where nssm >nul 2>&1
if %errorLevel% neq 0 (
    echo ERROR: NSSM is not installed or not in PATH
    echo Please install NSSM from: https://nssm.cc/
    echo Or download from: https://github.com/nssm/nssm/releases
    pause
    exit /b 1
)

REM Check if ssftpd executable exists
if not exist "%EXECUTABLE_PATH%" (
    echo ERROR: ssftpd executable not found at: %EXECUTABLE_PATH%
    echo Please ensure ssftpd is properly installed
    pause
    exit /b 1
)

REM Check if configuration exists
if not exist "%CONFIG_PATH%" (
    echo ERROR: Configuration file not found at: %CONFIG_PATH%
    echo Please ensure configuration files are properly installed
    pause
    exit /b 1
)

REM Create log directory if it doesn't exist
if not exist "%LOG_DIR%" (
    echo Creating log directory: %LOG_DIR%
    mkdir "%LOG_DIR%"
)

echo Installing %SERVICE_DISPLAY% service...

REM Remove existing service if it exists
sc query %SERVICE_NAME% >nul 2>&1
if %errorLevel% equ 0 (
    echo Removing existing service...
    nssm remove %SERVICE_NAME% confirm
    timeout /t 2 >nul
)

REM Install the service
echo Installing service...
nssm install %SERVICE_NAME% "%EXECUTABLE_PATH%"
if %errorLevel% neq 0 (
    echo ERROR: Failed to install service
    pause
    exit /b 1
)

REM Configure service parameters
echo Configuring service...
nssm set %SERVICE_NAME% AppParameters --config "%CONFIG_PATH%"
nssm set %SERVICE_NAME% AppDirectory "%WORKING_DIR%"
nssm set %SERVICE_NAME% DisplayName "%SERVICE_DISPLAY%"
nssm set %SERVICE_NAME% Description "Secure, configurable FTP server with modular architecture"
nssm set %SERVICE_NAME% Start SERVICE_AUTO_START
nssm set %SERVICE_NAME% AppStdout "%LOG_DIR%\ssftpd-service.log"
nssm set %SERVICE_NAME% AppStderr "%LOG_DIR%\ssftpd-service-error.log"

REM Set service dependencies
nssm set %SERVICE_NAME% DependOnService Tcpip

REM Set service recovery options
nssm set %SERVICE_NAME% AppRestartDelay 10000
nssm set %SERVICE_NAME% AppStopMethodSkip 0
nssm set %SERVICE_NAME% AppStopMethodConsole 1500
nssm set %SERVICE_NAME% AppStopMethodWindow 1500
nssm set %SERVICE_NAME% AppStopMethodThreads 1500

REM Set environment variables
nssm set %SERVICE_NAME% AppEnvironmentExtra SSFTPD_HOME="%WORKING_DIR%" SSFTPD_CONFIG="%CONFIG_PATH%" SSFTPD_LOGS="%LOG_DIR%"

echo Service installed successfully!
echo.
echo Service details:
echo   Name: %SERVICE_NAME%
echo   Display Name: %SERVICE_DISPLAY%
echo   Executable: %EXECUTABLE_PATH%
echo   Configuration: %CONFIG_PATH%
echo   Working Directory: %WORKING_DIR%
echo   Log Directory: %LOG_DIR%
echo.
echo To start the service:
echo   net start %SERVICE_NAME%
echo.
echo To stop the service:
echo   net stop %SERVICE_NAME%
echo.
echo To remove the service:
echo   nssm remove %SERVICE_NAME% confirm
echo.
pause
