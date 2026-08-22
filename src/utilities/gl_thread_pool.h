#pragma once

#include <atomic>
#include <condition_variable>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <queue>
#include <type_traits>
#include <utility>
#include <vector>

class QOffscreenSurface;
class QOpenGLContext;
class QThread;

class GLThreadPool {
    std::mutex mutex;
    std::condition_variable cv;
    std::queue<std::move_only_function<void()>> tasks;
    std::atomic<bool> stopping {false};

    std::vector<QThread*> worker_threads;
    std::vector<QOpenGLContext*> contexts;
    std::vector<QOffscreenSurface*> surfaces;

    void work_loop(
        QOpenGLContext* ctx,
        QOffscreenSurface* surface
    );

  public:
    void init(int count = 0);

    template<typename F, typename R = std::invoke_result_t<F>>
    std::future<R> submit(F&& f) {
        auto task =
            std::make_shared<std::packaged_task<R()>>(
                std::forward<F>(f)
            );

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

    void stop();

    ~GLThreadPool();
};

inline GLThreadPool gl_thread_pool;
