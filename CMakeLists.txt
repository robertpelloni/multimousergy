cmake_minimum_required(VERSION 3.20)
project(NetMux VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Driver Support (Optional)
option(ENABLE_INTERCEPTION "Enable Interception driver support" ON)
option(ENABLE_VIGEM "Enable ViGEmBus driver support" OFF)

# Library files
set(LIB_SOURCES
    src/DriverInterface.cpp
    src/InputEngine.cpp
    src/NetworkManager.cpp
    src/OverlayEngine.cpp
    src/D3D11Overlay.cpp
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
    target_link_directories(NetMuxLib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/lib/x64)
    target_link_libraries(NetMuxLib user32 gdi32 d3d11 dxgi ws2_32 d3dcompiler interception)

    if(ENABLE_INTERCEPTION)
        target_compile_definitions(NetMuxLib PRIVATE USE_INTERCEPTION)
    endif()

    if(ENABLE_VIGEM)
        target_compile_definitions(NetMuxLib PRIVATE USE_VIGEM)
    endif()

    # Copy interception.dll to output directory
    add_custom_command(TARGET NetMux POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/interception.dll"
        $<TARGET_FILE_DIR:NetMux>
    )
endif()

# Tests
enable_testing()
add_executable(NetMuxTests
    tests/test_main.cpp
    tests/test_multi_client.cpp
    tests/test_stress.cpp
    tests/test_network.cpp
    tests/test_sync.cpp
    tests/test_input.cpp
)
target_link_libraries(NetMuxTests NetMuxLib)
add_test(NAME CoreTests COMMAND NetMuxTests)

if(WIN32)
    add_custom_command(TARGET NetMuxTests POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/interception.dll"
        $<TARGET_FILE_DIR:NetMuxTests>
    )
endif()
