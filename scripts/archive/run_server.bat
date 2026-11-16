@echo off
echo Starting NetMux Server...
build\Release\NetMux.exe --server --port 5555 --boundary-x 1919 --left
pause
