module;

#include <chrono>

export module Timer;

export class Timer {
	std::chrono::steady_clock::time_point start_time;

  public:
	Timer() {
		start_time = std::chrono::steady_clock::now();
	}

	double elapsed_ms() {
		return (std::chrono::steady_clock::now() - start_time).count() / 1'000'000;
	}

	void reset() {
		start_time = std::chrono::steady_clock::now();
	}
};