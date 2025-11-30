#pragma once

#include "Core/Common.hpp"
#include "Core/Meta/TypeTraits.hpp"

#include <future>
#include <type_traits>
#include <utility>

MOE_BEGIN_NAMESPACE

template<typename T, typename SchedulerT>
struct Future;

template<typename T, typename SchedulerT>
struct Promise {
public:
    using value_type = T;
    using scheduler_type = SchedulerT;

    Promise() = default;

    void setValue(const T& value) {
        m_promise.set_value(value);
    }

    void setValue(T&& value) {
        m_promise.set_value(std::move(value));
    }

    Future<T, SchedulerT> getFuture() {
        return Future<T, SchedulerT>(m_promise.get_future());
    }

private:
    std::promise<T> m_promise;
};

template<typename SchedulerT>
struct Promise<void, SchedulerT> {
public:
    using value_type = void;

    Promise() = default;

    void setValue() {
        m_promise.set_value();
    }

    Future<void, SchedulerT> getFuture() {
        return Future<void, SchedulerT>(m_promise.get_future());
    }

private:
    std::promise<void> m_promise;
};

namespace Detail {
    template<typename MaybeFutureT>
    struct IsFuture : Meta::FalseType {};

    template<typename T, typename SchedulerT>
    struct IsFuture<Future<T, SchedulerT>> : Meta::TrueType {};

    template<typename MaybeFutureT>
    constexpr bool IsFutureV = IsFuture<MaybeFutureT>::value;

    template<typename T, typename FinalU, typename SchedulerT, typename Fn>
    struct VoidPredecessorTask {
    public:
        VoidPredecessorTask(Future<T, SchedulerT> pred, SharedPtr<Promise<FinalU, SchedulerT>> promise, Fn&& f)
            : predecessor(std::move(pred)), promise(std::move(promise)), func(std::forward<Fn>(f)) {}

        void operator()() {
            predecessor.get();
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
        Future<T, SchedulerT> predecessor;
        std::decay_t<Fn> func;
        SharedPtr<Promise<FinalU, SchedulerT>> promise;
    };

    template<typename T, typename FinalU, typename SchedulerT, typename Fn>
    struct ValuePredecessorTask {
    public:
        ValuePredecessorTask(Future<T, SchedulerT> pred, SharedPtr<Promise<FinalU, SchedulerT>> promise, Fn&& f)
            : predecessor(std::move(pred)), promise(std::move(promise)), func(std::forward<Fn>(f)) {}

        void operator()() {
            T value = predecessor.get();
            using RawU = Meta::InvokeResultT<std::decay_t<Fn>, T>;

            if constexpr (!Detail::IsFutureV<RawU>) {
                if constexpr (Meta::IsVoidV<RawU>) {
                    func(value);
                    promise->setValue();
                } else {
                    FinalU res = func(value);
                    promise->setValue(std::move(res));
                }
            } else {
                RawU innerFuture = func(value);
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
        Future<T, SchedulerT> predecessor;
        std::decay_t<Fn> func;
        SharedPtr<Promise<FinalU, SchedulerT>> promise;
    };
}// namespace Detail

template<typename MaybeFuture>
struct UnwrapFuture {
    using type = MaybeFuture;
};

template<typename Inner, typename SchedulerT>
struct UnwrapFuture<Future<Inner, SchedulerT>> {
    using type = Inner;
};

template<typename MaybeFutureT>
using UnwrapFutureT = typename UnwrapFuture<MaybeFutureT>::type;


template<typename T, typename SchedulerT>
struct Future {
public:
    using value_type = T;
    using scheduler_type = SchedulerT;

    explicit Future(const std::shared_future<T>& future)
        : m_future(future) {}

    explicit Future(std::shared_future<T>&& future)
        : m_future(std::move(future)) {}

    T get() {
        return m_future.get();
    }

    bool isReady() const {
        return m_future.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
    }

    bool isValid() const {
        return m_future.valid();
    }

    template<typename Fn,
             typename RawU = Meta::ConditionalT<
                     Meta::IsVoidV<T>,
                     Meta::InvokeResultT<std::decay_t<Fn>>,
                     Meta::InvokeResultT<std::decay_t<Fn>, T>>,
             typename FinalU = UnwrapFutureT<std::decay_t<RawU>>>
    Future<FinalU, SchedulerT> then(Fn&& func) {
        auto promise = std::make_shared<Promise<FinalU, SchedulerT>>();
        auto nextFuture = promise->getFuture();

        if constexpr (Meta::IsVoidV<T>) {
            auto task = std::make_shared<Detail::VoidPredecessorTask<T, FinalU, SchedulerT, Fn>>(
                    *this, promise, std::forward<Fn>(func));
            SchedulerT::getInstance().schedule([task]() mutable {
                (*task)();
            });
        } else {
            auto task = std::make_shared<Detail::ValuePredecessorTask<T, FinalU, SchedulerT, Fn>>(
                    *this, promise, std::forward<Fn>(func));
            SchedulerT::getInstance().schedule([task]() mutable {
                (*task)();
            });
        }
        return Future<FinalU, SchedulerT>(std::move(nextFuture));
    }

private:
    std::shared_future<T> m_future;
};

template<typename SchedulerT>
struct Future<void, SchedulerT> {
public:
    using value_type = void;
    using scheduler_type = SchedulerT;

    explicit Future(const std::shared_future<void>& future)
        : m_future(future) {}

    explicit Future(std::shared_future<void>&& future)
        : m_future(std::move(future)) {}

    void get() {
        m_future.get();
    }

    bool isReady() const {
        return m_future.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
    }

    bool isValid() const {
        return m_future.valid();
    }

    template<typename Fn,
             typename RawU = Meta::InvokeResultT<std::decay_t<Fn>>,
             typename FinalU = UnwrapFutureT<std::decay_t<RawU>>>
    Future<FinalU, SchedulerT> then(Fn&& func) {
        auto promise = std::make_shared<Promise<FinalU, SchedulerT>>();
        auto nextFuture = promise->getFuture();

        auto task = std::make_unique<Detail::VoidPredecessorTask<void, FinalU, SchedulerT, Fn>>(
                *this, promise, std::forward<Fn>(func));
        SchedulerT::getInstance().schedule([task]() mutable {
            (*task)();
        });

        return Future<FinalU, SchedulerT>(std::move(nextFuture));
    }

private:
    std::shared_future<void> m_future;
};

MOE_END_NAMESPACE