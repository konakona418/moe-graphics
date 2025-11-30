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
}// namespace Detail

template<
        typename F,
        typename RawR = Meta::InvokeResultT<std::decay_t<F>>,
        typename UnwrappedR = UnwrapFutureT<std::decay_t<RawR>>>
Future<UnwrappedR, Scheduler> async(F&& task) {
    auto promise = std::make_shared<Promise<UnwrappedR>>();
    auto fut = promise->getFuture();

    Scheduler::getInstance().schedule([task = std::forward<F>(task), prom = promise]() mutable {
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

template<
        typename... Futures,
        typename = Meta::EnableIfT<
                sizeof...(Futures) >= 2 &&
                !Detail::IsVectorOfFuturesV<Vector<std::decay_t<Futures>...>>>>
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

template<
        typename... Futures,
        typename = Meta::EnableIfT<
                sizeof...(Futures) >= 2 &&
                !Detail::IsVectorOfFuturesV<Vector<std::decay_t<Futures>...>>>>
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
    using OutFuture = Future<Vector<T>, Scheduler>;

    struct WhenAllContext {
        std::atomic_size_t pendingTasks;
        std::mutex mutex;
        std::once_flag setFlag;
        Vector<T> results;

        explicit WhenAllContext(size_t taskCount)
            : pendingTasks(taskCount) {}
    };

    auto promise = std::make_shared<Promise<Vector<T>>>();
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