#include "Logger.hpp"
#include <chrono>
#include <iomanip>
#include <sstream>

std::mutex Logger::m_mutex;
std::vector<std::string> Logger::m_logs;
std::ofstream Logger::m_file;
bool Logger::m_initialized = false;

void Logger::Initialize() {
    if (!m_initialized) {
        m_file.open("netmux.log", std::ios::trunc);  // Fresh log each run
        m_initialized = true;
    }
}

void Logger::Log(LogLevel level, const std::string& message) {
    std::lock_guard<std::mutex> lock(m_mutex);
    Initialize();

    auto now = std::chrono::system_clock::now();
    auto timeT = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

    std::stringstream ss;
    ss << std::put_time(std::localtime(&timeT), "%Y-%m-%d %H:%M:%S") << "." << std::setfill('0') << std::setw(3) << ms.count();

    std::string prefix;
    switch (level) {
        case LogLevel::Info: prefix = " [INFO] "; break;
        case LogLevel::Warning: prefix = " [WARN] "; break;
        case LogLevel::Error: prefix = " [ERR ] "; break;
        case LogLevel::Security: prefix = " [SEC ] "; break;
        case LogLevel::Debug: prefix = " [DBG ] "; break;
    }

    std::string fullMsg = ss.str() + prefix + message;

    // Console
    if (level == LogLevel::Error) std::cerr << fullMsg << std::endl;
    else std::cout << fullMsg << std::endl;

    // File
    if (m_file.is_open()) {
        m_file << fullMsg << std::endl;
        m_file.flush();
    }

    // Memory
    m_logs.push_back(fullMsg);
    if (m_logs.size() > 500) m_logs.erase(m_logs.begin());
}

std::vector<std::string> Logger::GetRecentLogs(size_t count) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_logs.size() <= count) return m_logs;
    return std::vector<std::string>(m_logs.end() - count, m_logs.end());
}

void Logger::Clear() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_logs.clear();
}
