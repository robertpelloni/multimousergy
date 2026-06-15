#pragma once
#include <string>
#include <vector>
#include <mutex>
#include <fstream>
#include <iostream>

enum class LogLevel {
    Info,
    Warning,
    Error,
    Security,
    Debug
};

class Logger {
public:
    static void Log(LogLevel level, const std::string& message);
    static std::vector<std::string> GetRecentLogs(size_t count = 100);
    static void Clear();

private:
    static std::mutex m_mutex;
    static std::vector<std::string> m_logs;
    static std::ofstream m_file;
    static bool m_initialized;

    static void Initialize();
};
