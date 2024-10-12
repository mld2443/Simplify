#pragma once

#include <chrono>   // time_point, steady_clock, now, nanoseconds, duration_cast
#include <string>   // string


class Timer
{
public:
    Timer(const std::string &name);
    ~Timer();

    void lap() const;

private:
    void measure(const char *type) const;

    const std::chrono::time_point<std::chrono::steady_clock> start;
    const std::string name;
};
