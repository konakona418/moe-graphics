#pragma once

#include "Core/Common.hpp"
#include "Core/Meta/TypeTraits.hpp"
#include "Core/Task/Future.hpp"

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>

MOE_BEGIN_NAMESPACE

struct DefaultScheduler {
public:
    static void init(size_t threadCount = std::thread::hardware_concurrency()) {
        auto& inst = getInstance();
        threadCount = threadCount == 0 ? 1 : threadCount;
        std::call_once(inst.m_initFlag, [threadCount, &inst]() {
            Logger::info("Initializing Scheduler with {} threads", threadCount);
            inst.start(threadCount);
        });
    }

    static DefaultScheduler* get() {
        init();
        return &getInstance();
    }

    static void shutdown() {
        Logger::info("Shutting down Scheduler");

        auto& inst = getInstance();
        std::call_once(inst.m_shutdownFlag, [&inst]() {
            inst.stop();
        });
    }

    template<typename F>
    void schedule(F&& task) {
        {
            std::lock_guard<std::mutex> lk(m_mutex);
            if (!m_running) return;
            m_tasks.emplace(Function<void()>(std::forward<F>(task)));
        }
        m_cv.notify_one();
    }

    template<
            typename F,
            typename RawR = Meta::InvokeResultT<std::decay_t<F>>,
            typename UnwrappedR = UnwrapFutureT<std::decay_t<RawR>>>
    Future<UnwrappedR, DefaultScheduler> scheduleAsync(F&& task) {

        auto promise = std::make_shared<Promise<UnwrappedR>>();
        auto fut = promise->getFuture();

        schedule([task = std::forward<F>(task), prom = promise]() mutable {
            if constexpr (Detail::IsFutureV<RawR>) {
                auto innerFuture = task();
                if constexpr (Meta::IsVoidV<UnwrappedR>) {
                    innerFuture.get();
                    prom->setValue();
                } else {
                    auto v = innerFuture.get();
                    prom->setValue(std::move(v));
                }
            } else {
                if constexpr (Meta::IsVoidV<RawR>) {
                    task();
                    prom->setValue();
                } else {
                    auto v = task();
                    prom->setValue(std::move(v));
                }
            }
        });

        return Future<UnwrappedR, DefaultScheduler>(std::move(fut));
    }

    void schedule(Function<void()> task) {
        {
            std::lock_guard<std::mutex> lk(m_mutex);
            if (!m_running) return;
            m_tasks.emplace(std::move(task));
        }
        m_cv.notify_one();
    }

    size_t workerCount() const { return m_workers.size(); }

private:
    Vector<std::thread> m_workers;
    Queue<Function<void()>> m_tasks;
    std::mutex m_mutex;
    std::condition_variable m_cv;
    std::atomic_bool m_running{false};
    std::once_flag m_initFlag;
    std::once_flag m_shutdownFlag;

    DefaultScheduler() = default;
    ~DefaultScheduler() {
        stop();
    }

    DefaultScheduler(const DefaultScheduler&) = delete;
    DefaultScheduler& operator=(const DefaultScheduler&) = delete;

    static DefaultScheduler& getInstance() {
        static DefaultScheduler instance;
        return instance;
    }

    void start(size_t threadCount) {
        {
            std::lock_guard<std::mutex> lk(m_mutex);
            if (m_running) return;
            m_running = true;
        }

        m_workers.reserve(threadCount);
        for (size_t i = 0; i < threadCount; ++i) {
            m_workers.emplace_back([this]() {
                workerMain();
            });
        }
    }

    void stop() {
        {
            std::lock_guard<std::mutex> lk(m_mutex);
            if (!m_running) return;
            m_running = false;
        }
        m_cv.notify_all();
        for (auto& t: m_workers) {
            if (t.joinable()) t.join();
        }
        m_workers.clear();

        Queue<Function<void()>> empty;
        {
            std::lock_guard<std::mutex> lk(m_mutex);
            std::swap(m_tasks, empty);
        }
    }

    void workerMain() {
        while (true) {
            Function<void()> task;
            {
                std::unique_lock<std::mutex> lk(m_mutex);
                m_cv.wait(lk, [this]() { return !m_running || !m_tasks.empty(); });
                if (!m_running && m_tasks.empty()) return;
                task = std::move(m_tasks.front());
                m_tasks.pop();
            }

            task();
        }
    }
};

MOE_END_NAMESPACE