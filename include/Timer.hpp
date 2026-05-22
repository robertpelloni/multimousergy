#pragma once
#include <chrono>

class Timer {
public:
    Timer();
    void Reset();
    double ElapsedMilliseconds() const;
    long long ElapsedMicroseconds() const;

private:
    std::chrono::time_point<std::chrono::high_resolution_clock> m_startTime;
};
