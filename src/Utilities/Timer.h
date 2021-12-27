#include <chrono>

class Timer {
	std::chrono::steady_clock::time_point start_time;

  public:
	Timer();

	double elapsed_ms();
	void reset();
};