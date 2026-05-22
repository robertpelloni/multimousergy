#include "NetMuxFramework.hpp"
#include <iostream>
#include <algorithm>
#include <thread>
#include <chrono>

#ifdef _WIN32
#include <windows.h>
#endif

NetMuxFramework::NetMuxFramework()
    : m_running(false), m_remoteX(0), m_remoteY(0), m_lastSyncTime(0) {}

NetMuxFramework::~NetMuxFramework() {
    Shutdown();
}

bool NetMuxFramework::Initialize(const AppSettings& settings) {
    m_settings = settings;

    if (m_settings.isServer) {
        if (!m_network.StartServer(m_settings.port)) {
            std::cerr << "Failed to start server on port " << m_settings.port << std::endl;
            return false;
        }
    } else {
        if (!m_network.Connect(m_settings.remoteIp, m_settings.port)) {
            std::cerr << "Failed to connect to " << m_settings.remoteIp << ":" << m_settings.port << std::endl;
            return false;
        }
    }

    if (!m_driver.Initialize() || !m_input.Initialize(m_settings.inputConfig) || !m_overlay.Initialize()) {
        std::cerr << "Failed to initialize core components." << std::endl;
        return false;
    }

    // Default color
    m_overlay.SetColor(255, 0, 0);

    m_running = true;
    return true;
}

void NetMuxFramework::Run() {
    while (m_running) {
        m_input.Update();

        PerformLatencySync();
        ProcessOutgoingPackets();
        ProcessIncomingPackets();

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

void NetMuxFramework::Shutdown() {
    if (m_running) {
        m_running = false;
        m_input.Shutdown();
        m_driver.Shutdown();
        m_overlay.Shutdown();
        m_network.Shutdown();
    }
}

void NetMuxFramework::ProcessOutgoingPackets() {
    Packet outPkt;
    while (m_input.GetPendingPacket(outPkt)) {
        if (m_input.IsCaptured()) {
            m_network.SendPacket(outPkt);
        }
    }
}

void NetMuxFramework::ProcessIncomingPackets() {
    Packet inPkt;
    while (m_network.ReceivePacket(inPkt)) {
        if (inPkt.type == PacketType::Movement) {
            m_remoteX += inPkt.x;
            m_remoteY += inPkt.y;

            // Clamp to screen bounds
#ifdef _WIN32
            m_remoteX = std::max(0, std::min(m_remoteX, (int)GetSystemMetrics(SM_CXSCREEN)));
            m_remoteY = std::max(0, std::min(m_remoteY, (int)GetSystemMetrics(SM_CYSCREEN)));
#endif

            m_overlay.Render(m_remoteX, m_remoteY);
            m_driver.SendMouseMovement(inPkt.x, inPkt.y);
        } else if (inPkt.type == PacketType::Click) {
            m_input.PerformWarpClickRestore(m_remoteX, m_remoteY, inPkt.button, inPkt.down);
            m_driver.SendMouseButton(inPkt.button, inPkt.down);
        } else if (inPkt.type == PacketType::Sync) {
            if (m_settings.isServer) {
                m_network.SendPacket(inPkt);
            } else {
                double rtt = m_syncTimer.ElapsedMilliseconds();
                std::cout << "[Latency] RTT: " << rtt << " ms" << std::endl;
            }
        }
    }
}

void NetMuxFramework::PerformLatencySync() {
    if (m_loopTimer.ElapsedMilliseconds() - m_lastSyncTime > 1000.0) {
        Packet syncPkt = { PacketType::Sync, 0, 0, 0, false };
        m_network.SendPacket(syncPkt);
        m_syncTimer.Reset();
        m_lastSyncTime = m_loopTimer.ElapsedMilliseconds();
    }
}
