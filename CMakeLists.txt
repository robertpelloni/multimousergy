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
    src/Timer.cpp
    src/NetMuxFramework.cpp
    src/SyncModule.cpp
    src/ClipboardModule.cpp
)

# Library
add_library(NetMuxLib STATIC ${LIB_SOURCES})
target_include_directories(NetMuxLib PUBLIC include)

# Executable
add_executable(NetMux src/main.cpp)
target_link_libraries(NetMux NetMuxLib)
target_include_directories(NetMux PRIVATE include)

# Windows specific libraries
if(WIN32)
    target_link_libraries(NetMuxLib user32 gdi32 d3d11 dxgi ws2_32)
endif()

# Tests
enable_testing()
add_executable(NetMuxTests tests/test_main.cpp tests/test_multi_client.cpp)
target_link_libraries(NetMuxTests NetMuxLib)
add_test(NAME CoreTests COMMAND NetMuxTests)
