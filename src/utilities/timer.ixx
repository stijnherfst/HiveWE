export module Timer;

import std;

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

// Global accumulators for profiling resource loading phases
export inline std::atomic<std::int64_t> profile_casc_ns{ 0 };
export inline std::atomic<std::int64_t> profile_parse_ns{ 0 };
export inline std::atomic<std::int64_t> profile_gl_ns{ 0 };

export void profile_reset() {
	profile_casc_ns = 0;
	profile_parse_ns = 0;
	profile_gl_ns = 0;
}

export void profile_print() {
	const double casc_ms = profile_casc_ns.load() / 1'000'000.0;
	const double parse_ms = profile_parse_ns.load() / 1'000'000.0;
	const double gl_ms = profile_gl_ns.load() / 1'000'000.0;
	std::println("  CASC I/O:  {:>7.1f}ms", casc_ms);
	std::println("  Parsing:   {:>7.1f}ms", parse_ms);
	std::println("  GL upload: {:>7.1f}ms", gl_ms);
}

// RAII helper: measures duration of its scope and adds it to the given accumulator
export class ScopedTimer {
	std::chrono::steady_clock::time_point start;
	std::atomic<std::int64_t>& accumulator;

  public:
	explicit ScopedTimer(std::atomic<std::int64_t>& acc)
		: start(std::chrono::steady_clock::now()), accumulator(acc) {}

	~ScopedTimer() {
		accumulator += (std::chrono::steady_clock::now() - start).count();
	}
};