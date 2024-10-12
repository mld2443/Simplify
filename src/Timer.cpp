#include "Timer.h"

#include <cmath>    // log10, powf
#include <iostream> // cout, endl


using namespace std;

static const char *unitScales[4] = {"ns", "us", "ms", "s"};

Timer::Timer(const string &name)
    : name(name)
    , start(chrono::steady_clock::now())
{
    cout << "Start \"" << name << "\"." << endl;
}

Timer::~Timer() {
    measure("-Stop \"");
}

void Timer::lap() const {
    measure("--Lap \"");
}

void Timer::measure(const char* type) const {
    const auto end = chrono::steady_clock::now();
    const uint64_t elapsed = chrono::duration_cast<chrono::nanoseconds>(end - start).count();
    const uint32_t digits = static_cast<uint32_t>(log10(elapsed));
    const uint32_t scale = min(digits / 3u, 3u);
    const float trimmed = static_cast<float>(elapsed) / powf(1000.f, static_cast<float>(scale));
    const char *units = unitScales[scale];

    cout << type << name << "\", " << trimmed << units << " elapsed." << endl;
}
