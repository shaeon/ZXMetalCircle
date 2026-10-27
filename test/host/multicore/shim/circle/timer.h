#pragma once
#include <chrono>
class CTimer { public: static unsigned GetClockTicks () { return (unsigned) std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now ().time_since_epoch ()).count (); } };
