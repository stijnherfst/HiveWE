#include "Timer.h"

Timer::Timer() {
	start_time = std::chrono::steady_clock::now();
}

double Timer::elapsed_ms() {
	return (std::chrono::steady_clock::now() - start_time).count() / 1'000'000;
}

void Timer::reset() {
	start_time = std::chrono::steady_clock::now();
}