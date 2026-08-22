module;

#ifdef __linux__
#include "std_compat.h"
#endif

export module Timer;

#ifndef __linux__
import std;
#endif

export class Timer {
	std::chrono::steady_clock::time_point start_time;

  public:
	Timer() {
		start_time = std::chrono::steady_clock::now();
	}

	double elapsed_ms() {
		return static_cast<double>((std::chrono::steady_clock::now() - start_time).count()) / 1'000'000;
	}

	void reset() {
		start_time = std::chrono::steady_clock::now();
	}
};