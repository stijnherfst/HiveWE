module;

#include <QOffscreenSurface>
#include <QOpenGLContext>
#include <QSurfaceFormat>
#include <QThread>

export module GLThreadPool;

import std;

export class GLThreadPool {
	std::mutex mutex;
	std::condition_variable cv;
	std::queue<std::move_only_function<void()>> tasks;
	std::atomic<bool> stopping {false};

	std::vector<QThread*> worker_threads;
	std::vector<QOpenGLContext*> contexts;
	std::vector<QOffscreenSurface*> surfaces;

	void work_loop(QOpenGLContext* ctx, QOffscreenSurface* surface) {
		if (!ctx->makeCurrent(surface)) {
			std::println("GLThreadPool: failed to make context current on worker thread");
			return;
		}

		while (true) {
			std::move_only_function<void()> task;
			{
				std::unique_lock lock(mutex);
				cv.wait(lock, [&] {
					return stopping.load() || !tasks.empty();
				});
				if (stopping.load() && tasks.empty()) {
					break;
				}
				task = std::move(tasks.front());
				tasks.pop();
			}
			task();
		}

		ctx->doneCurrent();
	}

  public:
	// Must be called from the main GL thread after the GL context has been initialized.
	void init(int count = 0) {
		if (count == 0) {
			count = static_cast<int>(std::max(1u, std::thread::hardware_concurrency() - 1));
		}

		QOpenGLContext* global_ctx = QOpenGLContext::globalShareContext();
		if (!global_ctx) {
			std::println("GLThreadPool: globalShareContext is null — parallel loading disabled");
			return;
		}

		const QSurfaceFormat fmt = global_ctx->format();

		for (int i = 0; i < count; i++) {
			// Offscreen surfaces must be created on the main (GUI) thread
			auto* surface = new QOffscreenSurface();
			surface->setFormat(fmt);
			surface->create();
			surfaces.push_back(surface);

			// Create context on the main thread and share with the global share context
			auto* ctx = new QOpenGLContext();
			ctx->setFormat(fmt);
			ctx->setShareContext(global_ctx);
			if (!ctx->create()) {
				std::println("GLThreadPool: failed to create shared GL context");
				surfaces.pop_back();
				delete surface;
				delete ctx;
				continue;
			}
			contexts.push_back(ctx);

			// Create the worker thread, then move context ownership to it before starting.
			// This satisfies Qt's requirement that makeCurrent is called from the owning thread.
			QThread* thread = QThread::create([this, ctx, surface]() {
				work_loop(ctx, surface);
			});
			ctx->moveToThread(thread);
			thread->start();
			worker_threads.push_back(thread);
		}
	}

	template<typename F, typename R = std::invoke_result_t<F>>
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
		return !worker_threads.empty();
	}

	void stop() {
		{
			std::lock_guard lock(mutex);
			stopping = true;
		}
		cv.notify_all();
		for (auto* t : worker_threads) {
			t->wait();
			delete t;
		}
		worker_threads.clear();
		for (auto* ctx : contexts) {
			delete ctx;
		}
		contexts.clear();
		for (auto* s : surfaces) {
			delete s;
		}
		surfaces.clear();
		stopping = false;
	}

	~GLThreadPool() {
		stop();
	}
};

export inline GLThreadPool gl_thread_pool;
