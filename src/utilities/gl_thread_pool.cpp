#include "gl_thread_pool.h"

#include <algorithm>
#include <print>
#include <thread>

#include <glad/glad.h>

#include <QOffscreenSurface>
#include <QOpenGLContext>
#include <QSurfaceFormat>
#include <QThread>

void GLThreadPool::work_loop(
    QOpenGLContext* ctx,
    QOffscreenSurface* surface
) {
    if (!ctx->makeCurrent(surface)) {
        std::println(
            "GLThreadPool: failed to make context current on worker thread"
        );
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

        glFlush();
    }

    ctx->doneCurrent();
}

void GLThreadPool::init(int count) {
    if (count == 0) {
        count = static_cast<int>(
            std::max(
                1u,
                std::thread::hardware_concurrency() - 1
            )
        );
    }

    QOpenGLContext* global_ctx =
        QOpenGLContext::globalShareContext();

    if (!global_ctx) {
        std::println(
            "GLThreadPool: globalShareContext is null — parallel loading disabled"
        );
        return;
    }

    const QSurfaceFormat fmt = global_ctx->format();

    for (int i = 0; i < count; i++) {
        auto* surface = new QOffscreenSurface();

        surface->setFormat(fmt);
        surface->create();

        surfaces.push_back(surface);

        auto* ctx = new QOpenGLContext();

        ctx->setFormat(fmt);
        ctx->setShareContext(global_ctx);

        if (!ctx->create()) {
            std::println(
                "GLThreadPool: failed to create shared GL context"
            );

            surfaces.pop_back();

            delete surface;
            delete ctx;

            continue;
        }

        contexts.push_back(ctx);

        QThread* thread =
            QThread::create(
                [this, ctx, surface]() {
                    work_loop(ctx, surface);
                }
            );

        ctx->moveToThread(thread);
        thread->start();

        worker_threads.push_back(thread);
    }
}

void GLThreadPool::stop() {
    {
        std::lock_guard lock(mutex);
        stopping = true;
    }

    cv.notify_all();

    for (auto* thread : worker_threads) {
        thread->wait();
        delete thread;
    }

    worker_threads.clear();

    for (auto* ctx : contexts) {
        delete ctx;
    }

    contexts.clear();

    for (auto* surface : surfaces) {
        delete surface;
    }

    surfaces.clear();

    stopping = false;
}

GLThreadPool::~GLThreadPool() {
    stop();
}
