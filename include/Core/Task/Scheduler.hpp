#pragma once

#include "Core/Common.hpp"
#include "Core/Task/Future.hpp"

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>

MOE_BEGIN_NAMESPACE

struct Scheduler {
public:
    static Scheduler& getInstance();

    static void init(size_t threadCount = std::thread::hardware_concurrency());

    static void shutdown();

    void schedule(Function<void()> task);

    size_t workerCount() const { return m_workers.size(); }

    template<typename F>
    void schedule(F&& task) {
        schedule(Function<void()>(std::forward<F>(task)));
    }

private:
    Vector<std::thread> m_workers;
    Queue<Function<void()>> m_tasks;
    std::mutex m_mutex;
    std::condition_variable m_cv;
    std::atomic_bool m_running{false};
    std::once_flag m_initFlag;
    std::once_flag m_shutdownFlag;

    Scheduler() = default;

    ~Scheduler() {
        stop();
    }

    Scheduler(const Scheduler&) = delete;
    Scheduler& operator=(const Scheduler&) = delete;

    void start(size_t threadCount);

    void stop();

    void workerMain();
};

MOE_END_NAMESPACE