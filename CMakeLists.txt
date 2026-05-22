cmake_minimum_required(VERSION 3.20)
project(NetMux VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Library files
set(LIB_SOURCES
    src/DriverInterface.cpp
    src/InputEngine.cpp
    src/NetworkManager.cpp
    src/OverlayEngine.cpp
    src/ConfigManager.cpp
    src/ConfigGUI.cpp
)

# Include directories
include_directories(include)

# Library
add_library(NetMuxLib STATIC ${LIB_SOURCES})

# Executable
add_executable(NetMux src/main.cpp)
target_link_libraries(NetMux NetMuxLib)

# Windows specific libraries
if(WIN32)
    target_link_libraries(NetMuxLib user32 gdi32 d3d11 dxgi)
endif()

# Tests
enable_testing()
add_executable(NetMuxTests tests/test_main.cpp)
target_link_libraries(NetMuxTests NetMuxLib)
add_test(NAME CoreTests COMMAND NetMuxTests)
