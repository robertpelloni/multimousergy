@echo off
set /p SERVER_IP="Enter Server IP: "
echo Starting NetMux Client...
build\Release\NetMux.exe --client %SERVER_IP% --port 5555 --boundary-x 0
pause
