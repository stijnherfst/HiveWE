#pragma once

#include <array>
#include <cstdint>

/// Starts frames just in time for a refresh instead of right after the previous one, so input is read as late as possible.
/// Times are host ticks: QueryPerformanceCounter on Windows, which is also the clock GPU timestamps are calibrated to.
/// Only Windows has a vblank clock (DWM's composition timing); elsewhere supported() is false.
class FramePacer {
  public:
	[[nodiscard]]
	static bool supported();
	[[nodiscard]]
	static int64_t now();
	[[nodiscard]]
	static int64_t ticks_per_second();

	/// Reports that a frame started at `start`, and returns the time to measure its cost from: the start next_start() asked
	/// for when it started later than that, so the timer firing late counts against the budget too
	[[nodiscard]]
	int64_t frame_started(int64_t start);

	/// Reports a frame that started at `start` (as frame_started() returned it), was finished by the GPU at `ready`, and
	/// was meant to be ready by the refresh at `target`
	void frame_finished(int64_t start, int64_t ready, int64_t target);

	/// When to start the next frame: as late as recent frames allow while still finishing before a refresh. At most one
	/// frame per refresh. Until frames have been measured, or when they take about a refresh or more, it starts them
	/// right away, only capped to the refresh rate.
	[[nodiscard]]
	int64_t next_start(int64_t now);

	/// The refresh the frame started at the last next_start() is meant to be ready for
	[[nodiscard]]
	int64_t target() const {
		return last_target;
	}

	/// Forgets the measured frames, for when their cost changes wholesale (a new swapchain)
	void reset();

  private:
	// A frame's cost varies, so the budget is the slowest of the last second or so of frames
	std::array<int64_t, 64> durations {};
	size_t duration_count = 0;
	size_t duration_index = 0;

	int64_t last_start = 0;
	int64_t last_target = 0;

	// Statistics printed once a second with HIVEWE_PACING=log
	bool log = false;
	bool log_checked = false;
	int64_t log_since = 0;
	uint32_t log_frames = 0;
	uint32_t log_misses = 0;
	int64_t log_duration_sum = 0;
	int64_t log_budget = 0;
	int64_t log_max_lateness = 0;
};
