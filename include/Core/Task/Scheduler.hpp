#pragma once

#include "Core/Common.hpp"
#include "Core/Meta/TypeTraits.hpp"
#include "Core/Task/Future.hpp"

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>

MOE_BEGIN_NAMESPACE

namespace Detail {
    template<typename F, typename Tuple, size_t... I>
    void applyToTupleImpl(F&& f, Tuple&& t, std::index_sequence<I...>) {
        (f(std::get<I>(std::forward<Tuple>(t)), Meta::IntegralConstant<size_t, I>{}), ...);
    }

    template<typename F, typename... Args>
    void applyToTuple(F&& f, std::tuple<Args...>&& t) {
        constexpr size_t tupleSize = sizeof...(Args);
        applyToTupleImpl(
                std::forward<F>(f),
                std::forward<std::tuple<Args...>>(t),
                std::make_index_sequence<tupleSize>{});
    }
}// namespace Detail

struct Scheduler {
public:
    static void init(size_t threadCount = std::thread::hardware_concurrency());

    static void shutdown();

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
    Future<UnwrappedR, Scheduler> scheduleAsync(F&& task) {
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

        return Future<UnwrappedR, Scheduler>(std::move(fut));
    }

    void schedule(Function<void()> task);

    size_t workerCount() const { return m_workers.size(); }

    static Scheduler& getInstance() {
        static Scheduler instance;
        if (!instance.m_running.load()) {
            Logger::warn("Scheduler instance requested but not initialized. Forcing initialization");
            std::call_once(
                    instance.m_initFlag,
                    [instance = std::ref(instance)]() {
                        instance.get().start(std::thread::hardware_concurrency());
                    });
        }

        return instance;
    }

    template<typename... Futures>
    auto waitForAll(Futures&&... futures) {
        using ResultTuple = std::tuple<UnwrapFutureT<typename std::decay_t<Futures>::value_type>...>;
        using OutFuture = Future<ResultTuple, Scheduler>;

        struct WaitForAllContext {
            std::atomic_size_t pendingTasks;
            std::mutex mutex;
            std::once_flag setFlag;
            ResultTuple results;

            explicit WaitForAllContext(size_t taskCount)
                : pendingTasks(taskCount) {}
        };

        auto promise = std::make_shared<Promise<ResultTuple>>();
        OutFuture outFuture = promise->getFuture();

        auto context = std::make_shared<WaitForAllContext>(sizeof...(Futures));
        auto bindTasks = [context, promise](auto&& future, auto idxValue) {
            future.then([context, promise, idxValue](auto&& value) {
                {
                    std::scoped_lock<std::mutex> lk(context->mutex);
                    std::get<decltype(idxValue)::value>(context->results) = std::move(value);
                }

                if (context->pendingTasks.fetch_sub(1) == 1) {
                    std::call_once(context->setFlag, [context, promise]() {
                        promise->setValue(std::move(context->results));
                    });
                }
            });
        };

        Detail::applyToTuple(bindTasks, std::make_tuple(std::forward<Futures>(futures)...));
        return outFuture;
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