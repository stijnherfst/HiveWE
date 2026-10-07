#include "frame_pacer.h"

#include <QString>

#ifdef _WIN32
	#define NOMINMAX
	#define WIN32_LEAN_AND_MEAN
	#include <windows.h>
	#include <dwmapi.h>
#endif

#include <algorithm>
#include <print>

namespace {
	struct VblankTiming {
		int64_t last_vblank;
		int64_t period;
	};

	bool vblank_timing(VblankTiming& timing) {
#ifdef _WIN32
		DWM_TIMING_INFO info = {.cbSize = sizeof(DWM_TIMING_INFO)};
		if (FAILED(DwmGetCompositionTimingInfo(nullptr, &info)) || info.qpcRefreshPeriod == 0) {
			return false;
		}
		timing.last_vblank = static_cast<int64_t>(info.qpcVBlank);
		timing.period = static_cast<int64_t>(info.qpcRefreshPeriod);
		return true;
#else
		return false;
#endif
	}

	int64_t ceil_div(const int64_t a, const int64_t b) {
		return a / b + (a % b > 0 ? 1 : 0);
	}
} // namespace

bool FramePacer::supported() {
#ifdef _WIN32
	return true;
#else
	return false;
#endif
}

int64_t FramePacer::now() {
#ifdef _WIN32
	LARGE_INTEGER counter;
	QueryPerformanceCounter(&counter);
	return counter.QuadPart;
#else
	return 0;
#endif
}

int64_t FramePacer::ticks_per_second() {
#ifdef _WIN32
	LARGE_INTEGER frequency;
	QueryPerformanceFrequency(&frequency);
	return frequency.QuadPart;
#else
	return 1;
#endif
}

int64_t FramePacer::frame_started(const int64_t start) {
	if (last_target == 0) {
		return start;
	}
	log_max_lateness = std::max(log_max_lateness, start - last_start);
	return std::min(start, last_start);
}

void FramePacer::frame_finished(const int64_t start, const int64_t ready, const int64_t target) {
	const int64_t duration = std::max<int64_t>(ready - start, 0);
	durations[duration_index] = duration;
	duration_index = (duration_index + 1) % durations.size();
	duration_count = std::min(duration_count + 1, durations.size());

	if (!log_checked) {
		log_checked = true;
		log = qEnvironmentVariable("HIVEWE_PACING").compare("log", Qt::CaseInsensitive) == 0;
	}
	if (!log) {
		return;
	}
	log_frames++;
	log_duration_sum += duration;
	if (target != 0 && ready > target) {
		log_misses++;
	}
	const int64_t frequency = ticks_per_second();
	if (log_since == 0) {
		log_since = ready;
	} else if (ready - log_since >= frequency) {
		const double to_ms = 1000.0 / static_cast<double>(frequency);
		std::println(
			"FramePacer: {} frames, start to GPU done {:.2f} ms average, budget {:.2f} ms, started up to {:.2f} ms late, {} missed their refresh",
			log_frames,
			static_cast<double>(log_duration_sum) / log_frames * to_ms,
			static_cast<double>(log_budget) * to_ms,
			static_cast<double>(log_max_lateness) * to_ms,
			log_misses
		);
		log_max_lateness = 0;
		log_since = ready;
		log_frames = 0;
		log_misses = 0;
		log_duration_sum = 0;
	}
}

int64_t FramePacer::next_start(const int64_t now) {
	VblankTiming timing;
	if (!vblank_timing(timing)) {
		last_start = now;
		last_target = 0;
		return now;
	}
	const int64_t period = timing.period;

	// Covers the compositor picking the frame up; frame costs already include the timer firing late
	const int64_t margin = ticks_per_second() / 1'000;
	const int64_t slowest = duration_count == 0 ? period : *std::max_element(durations.begin(), durations.begin() + duration_count);
	const int64_t budget = slowest + margin;
	log_budget = budget;

	if (budget >= period * 9 / 10) {
		// No time to spare in a refresh, so start right away as FIFO would, at most once per refresh
		const int64_t start = std::max(now, last_start + period * 95 / 100);
		last_start = start;
		last_target = 0;
		return start;
	}

	// The first refresh this frame can still make, but never the same one as the previous frame
	int64_t target = timing.last_vblank + ceil_div(now + budget - timing.last_vblank, period) * period;
	if (last_target != 0 && target < last_target + period / 2) {
		target += ceil_div(last_target + period / 2 - target, period) * period;
	}

	last_target = target;
	last_start = target - budget;
	return last_start;
}

void FramePacer::reset() {
	duration_count = 0;
	duration_index = 0;
	last_start = 0;
	last_target = 0;
}
