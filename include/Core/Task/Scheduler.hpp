#pragma once

#include "Core/Common.hpp"
#include "Core/Meta/TypeTraits.hpp"
#include "Core/Meta/Util.hpp"
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

    template<
            typename F,
            typename RawR = Meta::InvokeResultT<std::decay_t<F>>,
            typename UnwrappedR = UnwrapFutureT<std::decay_t<RawR>>>
    Future<UnwrappedR, Scheduler> async(F&& task) {
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

    template<typename... Futures>
    auto whenAll(Futures&&... futures) {
        using ResultTuple = std::tuple<UnwrapFutureT<typename std::decay_t<Futures>::value_type>...>;
        using OutFuture = Future<ResultTuple, Scheduler>;

        struct WhenAllContext {
            std::atomic_size_t pendingTasks;
            std::mutex mutex;
            std::once_flag setFlag;
            ResultTuple results;

            explicit WhenAllContext(size_t taskCount)
                : pendingTasks(taskCount) {}
        };

        auto promise = std::make_shared<Promise<ResultTuple>>();
        OutFuture outFuture = promise->getFuture();

        auto context = std::make_shared<WhenAllContext>(sizeof...(Futures));
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

        applyToTuple(bindTasks, std::make_tuple(std::forward<Futures>(futures)...));
        return outFuture;
    }

    template<typename... Futures>
    auto whenAny(Futures&&... futures) {
        using ResultVariant = Variant<UnwrapFutureT<typename std::decay_t<Futures>::value_type>...>;
        using OutFuture = Future<ResultVariant, Scheduler>;

        struct WhenAnyContext {
            std::atomic_bool isSet{false};
            std::mutex mutex;
            std::once_flag setFlag;
            ResultVariant result;
        };

        auto promise = std::make_shared<Promise<ResultVariant>>();
        OutFuture outFuture = promise->getFuture();

        auto context = std::make_shared<WhenAnyContext>();
        auto bindTasks = [context, promise](auto&& future, auto /*idxValue*/) {
            future.then([context, promise](auto&& value) {
                if (!context->isSet.load()) {
                    std::call_once(context->setFlag, [context, promise, value = std::move(value)]() mutable {
                        context->isSet.store(true);
                        promise->setValue(ResultVariant(std::move(value)));
                    });
                }
            });
        };

        applyToTuple(bindTasks, std::make_tuple(std::forward<Futures>(futures)...));
        return outFuture;
    }

    template<typename T>
    auto whenAll(Vector<Future<T, Scheduler>>&& futures) {
        using OutFuture = Future<T, Scheduler>;

        struct WhenAllContext {
            std::atomic_size_t pendingTasks;
            std::once_flag setFlag;
            Vector<T> results;

            explicit WhenAllContext(size_t taskCount)
                : pendingTasks(taskCount) {}
        };

        auto promise = std::make_shared<Promise<Vector<T>>>();
        OutFuture outFuture = promise->getFuture();

        auto context = std::make_shared<WhenAllContext>(futures.size());
        for (auto& future: futures) {
            future.then([context, promise, future = std::move(future)]() {
                {
                    std::scoped_lock<std::mutex> lk(context->mutex);
                    context->results.push_back(future.get());
                }

                if (context->pendingTasks.fetch_sub(1) == 1) {
                    std::call_once(context->setFlag, [context, promise]() {
                        promise->setValue(std::move(context->results));
                    });
                }
            });
        }

        return outFuture;
    }

    template<typename T>
    auto whenAny(Vector<Future<T, Scheduler>>&& futures) {
        using OutFuture = Future<T, Scheduler>;

        struct WhenAnyContext {
            std::atomic_bool isSet{false};
            std::once_flag setFlag;
        };

        auto promise = std::make_shared<Promise<T>>();
        OutFuture outFuture = promise->getFuture();

        auto context = std::make_shared<WhenAnyContext>();
        for (auto& future: futures) {
            future.then([context, promise, future = std::move(future)]() {
                if (!context->isSet.load()) {
                    std::call_once(context->setFlag, [context, promise, future = std::move(future)]() mutable {
                        context->isSet.store(true);
                        promise->setValue(future.get());
                    });
                }
            });
        }

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