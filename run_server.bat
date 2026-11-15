@echo off
setlocal
echo ==============================================
echo [MultiMousergy] Starting Server Host
echo ==============================================

if exist build\Release\NetMux.exe (
    build\Release\NetMux.exe --server --port 5555 --boundary-x 1919 --left
) else if exist build\NetMux.exe (
    build\NetMux.exe --server --port 5555 --boundary-x 1919 --left
) else (
    echo [Error] NetMux executable not found. Please run build.bat first.
    pause
    exit /b 1
)

pause
