#pragma once

#include "Core/Common.hpp"
#include "Core/Meta/TypeTraits.hpp"
#include "Core/Meta/Util.hpp"
#include "Core/Task/Scheduler.hpp"

MOE_BEGIN_NAMESPACE

namespace Detail {
    template<typename MaybeVectorOfFutures>
    struct IsVectorOfFutures : Meta::FalseType {};

    template<typename T, typename SchedulerT>
    struct IsVectorOfFutures<Vector<Future<T, SchedulerT>>> : Meta::TrueType {};

    template<typename MaybeVectorOfFutures>
    constexpr bool IsVectorOfFuturesV = IsVectorOfFutures<MaybeVectorOfFutures>::value;

    template<typename FinalU, typename SchedulerT, typename Fn>
    struct AsyncTask {
    public:
        AsyncTask(SharedPtr<Promise<FinalU, SchedulerT>> promise, Fn&& f)
            : promise(std::move(promise)), func(std::forward<Fn>(f)) {}

        void operator()() {
            using RawU = Meta::InvokeResultT<std::decay_t<Fn>>;

            if constexpr (!Detail::IsFutureV<RawU>) {
                if constexpr (Meta::IsVoidV<RawU>) {
                    func();
                    promise->setValue();
                } else {
                    FinalU res = func();
                    promise->setValue(std::move(res));
                }
            } else {
                RawU innerFuture = func();
                innerFuture
                        .then([promise = this->promise](auto&& inner) {
                            if constexpr (Meta::IsVoidV<FinalU>) {
                                promise->setValue();
                            } else {
                                promise->setValue(std::forward<decltype(inner)>(inner));
                            }
                        });
            }
        }

    private:
        std::decay_t<Fn> func;
        SharedPtr<Promise<FinalU, SchedulerT>> promise;
    };
}// namespace Detail

template<
        typename F,
        typename SchedulerT = ThreadPoolScheduler,
        typename RawR = Meta::InvokeResultT<std::decay_t<F>>,
        typename UnwrappedR = UnwrapFutureT<std::decay_t<RawR>>>
Future<UnwrappedR, SchedulerT> async(F&& task) {
    auto promise = std::make_shared<Promise<UnwrappedR, SchedulerT>>();
    auto fut = promise->getFuture();

    auto asyncTask = std::make_shared<Detail::AsyncTask<UnwrappedR, SchedulerT, F>>(
            promise, std::forward<F>(task));
    SchedulerT::getInstance().schedule([asyncTask]() mutable {
        (*asyncTask)();
    });

    return Future<UnwrappedR, SchedulerT>(std::move(fut));
}

template<
        typename F,
        typename RawR = Meta::InvokeResultT<std::decay_t<F>>,
        typename UnwrappedR = UnwrapFutureT<std::decay_t<RawR>>>
Future<UnwrappedR, MainScheduler> asyncOnMainThread(F&& task) {
    auto& mainThreadDispatcher = MainScheduler::getInstance();
    auto promise = std::make_shared<Promise<UnwrappedR, MainScheduler>>();
    auto fut = promise->getFuture();

    auto asyncTask = std::make_shared<Detail::AsyncTask<UnwrappedR, MainScheduler, F>>(
            promise, std::forward<F>(task));
    mainThreadDispatcher.schedule([asyncTask]() mutable {
        (*asyncTask)();
    });

    return Future<UnwrappedR, MainScheduler>(std::move(fut));
}

template<
        typename... Futures,
        typename = Meta::EnableIfT<
                sizeof...(Futures) >= 2 &&
                !Detail::IsVectorOfFuturesV<Vector<std::decay_t<Futures>...>>>>
auto whenAll(Futures&&... futures) {
    using ResultTuple = std::tuple<UnwrapFutureT<typename std::decay_t<Futures>::value_type>...>;

    static_assert(
            (std::is_same_v<
                     typename std::decay_t<Futures>::scheduler_type,
                     typename std::decay_t<std::tuple_element_t<0, std::tuple<Futures...>>>::scheduler_type> &&
             ...),
            "All Futures passed to whenAll must have the same SchedulerT");

    using SchedulerT = std::decay_t<typename std::decay_t<std::tuple_element_t<0, std::tuple<Futures...>>>::scheduler_type>;
    using OutFuture = Future<ResultTuple, SchedulerT>;

    struct WhenAllContext {
        std::atomic_size_t pendingTasks;
        std::mutex mutex;
        std::once_flag setFlag;
        ResultTuple results;

        explicit WhenAllContext(size_t taskCount)
            : pendingTasks(taskCount) {}
    };

    auto promise = std::make_shared<Promise<ResultTuple, SchedulerT>>();
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

template<
        typename... Futures,
        typename = Meta::EnableIfT<
                sizeof...(Futures) >= 2 &&
                !Detail::IsVectorOfFuturesV<Vector<std::decay_t<Futures>...>>>>
auto whenAny(Futures&&... futures) {
    using ResultVariant = Variant<UnwrapFutureT<typename std::decay_t<Futures>::value_type>...>;

    static_assert(
            (std::is_same_v<
                     typename std::decay_t<Futures>::scheduler_type,
                     typename std::decay_t<std::tuple_element_t<0, std::tuple<Futures...>>>::scheduler_type> &&
             ...),
            "All Futures passed to whenAny must have the same SchedulerT");
    using SchedulerT = std::decay_t<typename std::decay_t<std::tuple_element_t<0, std::tuple<Futures...>>>::scheduler_type>;
    using OutFuture = Future<ResultVariant, SchedulerT>;

    struct WhenAnyContext {
        std::atomic_bool isSet{false};
        std::mutex mutex;
        std::once_flag setFlag;
        ResultVariant result;
    };

    auto promise = std::make_shared<Promise<ResultVariant, SchedulerT>>();
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

template<typename T, typename SchedulerT>
auto whenAll(Vector<Future<T, SchedulerT>>&& futures) {
    using OutFuture = Future<Vector<T>, SchedulerT>;

    struct WhenAllContext {
        std::atomic_size_t pendingTasks;
        std::mutex mutex;
        std::once_flag setFlag;
        Vector<T> results;

        explicit WhenAllContext(size_t taskCount)
            : pendingTasks(taskCount) {}
    };

    auto promise = std::make_shared<Promise<Vector<T>, SchedulerT>>();
    OutFuture outFuture = promise->getFuture();

    auto context = std::make_shared<WhenAllContext>(futures.size());
    for (auto& future: futures) {
        future.then([context, promise](auto&& value) mutable {
            {
                std::scoped_lock<std::mutex> lk(context->mutex);
                context->results.push_back(std::move(value));
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

template<typename T, typename SchedulerT>
auto whenAny(Vector<Future<T, SchedulerT>>&& futures) {
    using OutFuture = Future<T, SchedulerT>;

    struct WhenAnyContext {
        std::atomic_bool isSet{false};
        std::once_flag setFlag;
    };

    auto promise = std::make_shared<Promise<T, SchedulerT>>();
    OutFuture outFuture = promise->getFuture();

    auto context = std::make_shared<WhenAnyContext>();
    for (auto& future: futures) {
        future.then([context, promise](auto&& value) {
            if (!context->isSet.load()) {
                std::call_once(context->setFlag, [context, promise, value = std::move(value)]() mutable {
                    context->isSet.store(true);
                    promise->setValue(std::move(value));
                });
            }
        });
    }

    return outFuture;
}

MOE_END_NAMESPACE