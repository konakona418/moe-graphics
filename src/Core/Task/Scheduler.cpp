#include "Core/Task/Scheduler.hpp"

MOE_BEGIN_NAMESPACE

void Scheduler::init(size_t threadCount) {
    auto& inst = getInstance();
    threadCount = threadCount == 0 ? 1 : threadCount;
    std::call_once(inst.m_initFlag, [threadCount, &inst]() {
        Logger::info("Initializing scheduler with {} threads", threadCount);
        inst.start(threadCount);
    });
}

void Scheduler::shutdown() {
    Logger::info("Shutting down scheduler");

    auto& inst = getInstance();
    std::call_once(inst.m_shutdownFlag, [&inst]() {
        inst.stop();
    });
}

void Scheduler::schedule(Function<void()> task) {
    {
        std::lock_guard<std::mutex> lk(m_mutex);
        if (!m_running) return;
        m_tasks.emplace(std::move(task));
    }
    m_cv.notify_one();
}

void Scheduler::start(size_t threadCount) {
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

void Scheduler::stop() {
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

void Scheduler::workerMain() {
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

MOE_END_NAMESPACE