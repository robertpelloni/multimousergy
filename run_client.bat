@echo off
setlocal
echo ==============================================
echo [MultiMousergy] Starting Client Peer
echo ==============================================

set /p SERVER_IP="Enter Server Host IP (default 127.0.0.1): "
if "%SERVER_IP%"=="" set SERVER_IP=127.0.0.1

if exist build\Release\NetMux.exe (
    build\Release\NetMux.exe --client %SERVER_IP% --port 5555 --boundary-x 0 --right
) else if exist build\NetMux.exe (
    build\NetMux.exe --client %SERVER_IP% --port 5555 --boundary-x 0 --right
) else (
    echo [Error] NetMux executable not found. Please run build.bat first.
    pause
    exit /b 1
)

pause
