export module ThreadPool;

import std;

/// Worker threads for loading work, such as constructing meshes and their GPU resources in parallel.
export class ThreadPool {
	std::mutex mutex;
	std::condition_variable cv;
	std::queue<std::move_only_function<void()>> tasks;
	bool stopping = false;

	std::vector<std::thread> workers;

	void work_loop() {
		while (true) {
			std::move_only_function<void()> task;
			{
				std::unique_lock lock(mutex);
				cv.wait(lock, [&] {
					return stopping || !tasks.empty();
				});
				if (stopping && tasks.empty()) {
					return;
				}
				task = std::move(tasks.front());
				tasks.pop();
			}
			task();
		}
	}

  public:
	/// Starts `count` workers, or one fewer than the number of hardware threads when 0
	void init(int count = 0) {
		if (count == 0) {
			count = static_cast<int>(std::max(1u, std::thread::hardware_concurrency() - 1));
		}
		for (int i = 0; i < count; i++) {
			workers.emplace_back([this] {
				work_loop();
			});
		}
	}

	template <typename F, typename R = std::invoke_result_t<F>>
	std::future<R> submit(F&& f) {
		auto task = std::make_shared<std::packaged_task<R()>>(std::forward<F>(f));
		std::future<R> future = task->get_future();
		{
			std::lock_guard lock(mutex);
			tasks.push([task]() mutable {
				(*task)();
			});
		}
		cv.notify_one();
		return future;
	}

	bool is_initialized() const {
		return !workers.empty();
	}

	void stop() {
		{
			std::lock_guard lock(mutex);
			stopping = true;
		}
		cv.notify_all();
		for (auto& worker : workers) {
			worker.join();
		}
		workers.clear();
		stopping = false;
	}

	~ThreadPool() {
		stop();
	}
};

export inline ThreadPool thread_pool;
