cmake_minimum_required(VERSION 3.20)
project(NetMux VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Driver Support (Optional)
option(ENABLE_INTERCEPTION "Enable Interception driver support" OFF)
option(ENABLE_VIGEM "Enable ViGEmBus driver support" OFF)

# Native Driver Support
option(NETMUX_USE_NATIVE_DRIVERS "Link against native Interception/ViGEm SDKs" OFF)
if(NETMUX_USE_NATIVE_DRIVERS)
    add_definitions(-DNETMUX_USE_NATIVE_DRIVERS)
endif()

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
    src/D3D11Overlay.cpp
    src/AuthModule.cpp
    src/AuthService.cpp
    src/PacketSerializer.cpp
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
    add_definitions(-DNOMINMAX)
    target_link_libraries(NetMuxLib user32 gdi32 d3d11 dxgi ws2_32 d3dcompiler bcrypt)

    if(ENABLE_INTERCEPTION)
        find_library(INTERCEPTION_LIB interception)
        if(INTERCEPTION_LIB)
            target_link_libraries(NetMuxLib ${INTERCEPTION_LIB})
            target_compile_definitions(NetMuxLib PRIVATE USE_INTERCEPTION)
        endif()
    endif()

    if(ENABLE_VIGEM)
        find_package(ViGEmClient QUIET)
        if(ViGEmClient_FOUND)
            target_link_libraries(NetMuxLib ViGEmClient::ViGEmClient)
            target_compile_definitions(NetMuxLib PRIVATE USE_VIGEM)
        endif()
    endif()
endif()

# Linux specific libraries
if(UNIX AND NOT APPLE)
    find_package(OpenSSL REQUIRED)
    find_package(X11 REQUIRED)
    target_link_libraries(NetMuxLib OpenSSL::Crypto ${X11_LIBRARIES})
    target_include_directories(NetMuxLib PRIVATE ${X11_INCLUDE_DIR})
endif()

# Tests
enable_testing()
add_executable(NetMuxTests
    tests/test_main.cpp
    tests/test_auth.cpp
    tests/test_auth_service.cpp
    tests/test_multi_client.cpp
    tests/test_stress.cpp
    tests/test_network.cpp
    tests/test_sync.cpp
    tests/test_input.cpp
    tests/test_clipboard.cpp
    tests/test_sync_consistency.cpp
)
target_link_libraries(NetMuxTests NetMuxLib)
add_test(NAME CoreTests COMMAND NetMuxTests)
