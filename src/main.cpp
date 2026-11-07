#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <windows.h>
#endif
#include <iostream>
#include <algorithm>
#include <sstream>
#include "NetMuxFramework.hpp"
#include "SystemTray.hpp"

#include <thread>
#include <chrono>
#include <atomic>

int main(int argc, char* argv[]) {
    #ifdef _WIN32
    SetProcessDPIAware();
    #endif

    // Show help on --help or -h
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--help" || arg == "-h") {
            std::cout << "MultiMousergy v0.1.91-alpha - Cross-Network Multi-Cursor System\n\n"
                      << "Usage: NetMux.exe [options]\n\n"
                      << "  --server                  Run as server host (default: client)\n"
                      << "  --client <ip>             Connect to server host at <ip>\n"
                      << "  --port <n>                Use port <n> (default: 5555)\n"
                      << "  --boundary-x <n>          Screen edge crossing coordinate X\n"
                      << "  --boundary-y <n>          Screen edge crossing coordinate Y\n"
                      << "  --left                    Cursor crosses to left monitor\n"
                      << "  --right                   Cursor crosses to right monitor\n"
                      << "  --d3d11                   Enable D3D11 spatial viewport & DXGI capture\n"
                      << "  --spatial                 Enable 3D spatial viewport transformations\n"
                      << "  --headless                Enable stdout JSON telemetry streaming\n"
                      << "  --benchmark               Run performance benchmarking loop\n"
                      << "  --auto-connect            Auto-connect on startup\n"
                      << "  --key <secret>            Pre-shared security key (SHA-256 HMAC)\n"
                      << "  --driver-type <0|1|2>     Virtual HID Driver (0:Interception, 1:ViGEm, 2:Auto)\n"
                      << "  --display-name <name>     Friendly workstation display name\n"
                      << "  --session-name <name>     Collaborative session identifier\n"
                      << "  --group-name <name>       Multi-tenant group name\n"
                      << "  --group-id <n>            Multi-tenant group numeric ID\n"
                      << "  --peer-color <r,g,b>      Remote cursor RGB color tint (0-255)\n"
                      << "  --sel-color <r,g,b>       Selection box RGB color tint (0-255)\n"
                      << "  --cursor-theme <path>     Path to custom 32x32 BMP cursor texture\n"
                      << "  --cursor-scale <f>        Cursor size scaling multiplier (e.g. 1.25)\n"
                      << "  --help, -h                Show this help\n"
                      << std::endl;
            return 0;
        }
    }

    std::cout << "MultiMousergy starting..." << std::endl;

    ConfigManager configManager("netmux.cfg");
    AppSettings settings = { false, "127.0.0.1", 5555, {0, 0, false} };

    configManager.Load(settings);

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--server") settings.isServer = true;
        else if (arg == "--client" && i + 1 < argc) { settings.remoteIp = argv[++i]; settings.isServer = false; }
        else if (arg == "--port" && i + 1 < argc) settings.port = std::stoi(argv[++i]);
        else if (arg == "--boundary-x" && i + 1 < argc) settings.inputConfig.boundaryX = std::stoi(argv[++i]);
        else if (arg == "--boundary-y" && i + 1 < argc) settings.inputConfig.boundaryY = std::stoi(argv[++i]);
        else if (arg == "--left") settings.inputConfig.isLeft = true;
        else if (arg == "--right") settings.inputConfig.isLeft = false;
        else if (arg == "--d3d11") settings.useD3D11 = true;
        else if (arg == "--spatial") settings.spatialMode = true;
        else if (arg == "--headless") settings.headless = true;
        else if (arg == "--benchmark") settings.benchmarking = true;
        else if (arg == "--auto-connect") settings.autoConnect = true;
        else if ((arg == "--key" || arg == "-k") && i + 1 < argc) settings.securityKey = argv[++i];
        else if (arg == "--driver-type" && i + 1 < argc) settings.driverType = static_cast<NetMuxDriverType>(std::stoi(argv[++i]));
        else if (arg == "--display-name" && i + 1 < argc) settings.displayName = argv[++i];
        else if (arg == "--session-name" && i + 1 < argc) settings.sessionName = argv[++i];
        else if (arg == "--group-name" && i + 1 < argc) settings.groupName = argv[++i];
        else if (arg == "--group-id" && i + 1 < argc) settings.groupId = static_cast<unsigned int>(std::stoul(argv[++i]));
        else if (arg == "--cursor-theme" && i + 1 < argc) settings.cursorThemePath = argv[++i];
        else if (arg == "--cursor-scale" && i + 1 < argc) settings.cursorScale = std::stof(argv[++i]);
        else if (arg == "--peer-color" && i + 1 < argc) {
            std::stringstream ss(argv[++i]);
            std::string r, g, b;
            if (std::getline(ss, r, ',') && std::getline(ss, g, ',') && std::getline(ss, b, ',')) {
                settings.peerColorR = static_cast<unsigned char>(std::stoi(r));
                settings.peerColorG = static_cast<unsigned char>(std::stoi(g));
                settings.peerColorB = static_cast<unsigned char>(std::stoi(b));
            }
        }
        else if (arg == "--sel-color" && i + 1 < argc) {
            std::stringstream ss(argv[++i]);
            std::string r, g, b;
            if (std::getline(ss, r, ',') && std::getline(ss, g, ',') && std::getline(ss, b, ',')) {
                settings.selectionColorR = static_cast<unsigned char>(std::stoi(r));
                settings.selectionColorG = static_cast<unsigned char>(std::stoi(g));
                settings.selectionColorB = static_cast<unsigned char>(std::stoi(b));
            }
        }
    }

    NetMuxFramework framework;

    if (!framework.Initialize(settings)) {
        std::cerr << "Framework init failed." << std::endl;
        return 1;
    }

#ifdef __linux__
    framework.GetInputEngine().Initialize(settings.inputConfig, framework.m_xDisplay);
#else
    framework.GetInputEngine().Initialize(settings.inputConfig);
#endif

    framework.SetCursorColor(settings.peerColorR, settings.peerColorG, settings.peerColorB);

#ifdef _WIN32
    // Initialize system tray icon
    SystemTray tray;
    std::atomic<bool> trayExitRequested{false};
    {
        std::string mode = settings.isServer ? "Server" : "Client";
        std::string tooltip = "MultiMousergy - " + mode + " (v0.1.91-alpha)";
        tray.Initialize(GetModuleHandle(nullptr), tooltip, [&]() {
            std::cout << "[Tray] Exit requested." << std::endl;
            trayExitRequested.store(true);
        });
    }
#endif

    std::thread frameworkThread([&]() {
        framework.Run();
    });

#ifdef _WIN32
    // Track last tooltip update to avoid spamming Shell_NotifyIcon
    auto lastTipUpdate = std::chrono::steady_clock::now();
    MSG msg;
    while (framework.IsRunning() && !trayExitRequested.load()) {
        // Pump tray messages
        tray.PumpMessages();

        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            if (msg.message == WM_QUIT) break;
        } else {
            framework.GetInputEngine().Update();

            // Update tray tooltip every 2 seconds
            auto now = std::chrono::steady_clock::now();
            if (std::chrono::duration_cast<std::chrono::seconds>(now - lastTipUpdate).count() >= 2) {
                lastTipUpdate = now;
                auto peerCount = framework.GetSyncModule().GetAllPeers().size();
                std::string mode = settings.isServer ? "Server" : "Client";
                std::string tip = "MultiMousergy - " + mode + "\nPeers: " + std::to_string(peerCount);
                tray.SetTooltip(tip);
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }
#else
    while (framework.IsRunning()) {
        framework.GetInputEngine().Update();
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }
#endif

    framework.Shutdown();
    if (frameworkThread.joinable()) frameworkThread.join();

#ifdef _WIN32
    tray.Shutdown();
#endif

    return 0;
}
