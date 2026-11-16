@echo off
setlocal
echo ==============================================
echo [MultiMousergy] Building Backend & UI Client
echo ==============================================

if not exist build mkdir build
cd build
cmake ..
if %ERRORLEVEL% NEQ 0 (
    echo [Error] CMake configuration failed.
    exit /b %ERRORLEVEL%
)

cmake --build . --config Release
if %ERRORLEVEL% NEQ 0 (
    echo [Error] C++ build failed.
    exit /b %ERRORLEVEL%
)

cd ..
if exist ui\package.json (
    echo [Info] Installing UI frontend dependencies...
    cd ui && call npm install && cd ..
)

echo ==============================================
echo [MultiMousergy] Build succeeded!
echo ==============================================
pause
