#include "Timer.hpp"

Timer::Timer() {
    Reset();
}

void Timer::Reset() {
    m_startTime = std::chrono::high_resolution_clock::now();
}

double Timer::ElapsedMilliseconds() const {
    auto endTime = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> elapsed = endTime - m_startTime;
    return elapsed.count();
}

long long Timer::ElapsedMicroseconds() const {
    auto endTime = std::chrono::high_resolution_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(endTime - m_startTime);
    return elapsed.count();
}
