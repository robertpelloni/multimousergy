@echo off
echo Building NetMux...
if not exist build mkdir build
cd build
cmake ..
cmake --build . --config Release
echo Build complete.
pause
