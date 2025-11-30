#pragma once

#include "Core/Common.hpp"
#include "Core/Meta/TypeTraits.hpp"

#include <future>
#include <type_traits>
#include <utility>

MOE_BEGIN_NAMESPACE

struct Scheduler;

template<typename T, typename SchedulerT = Scheduler>
struct Future;

template<typename T>
struct Promise {
public:
    using value_type = T;

    Promise() = default;

    void setValue(const T& value) {
        m_promise.set_value(value);
    }

    void setValue(T&& value) {
        m_promise.set_value(std::move(value));
    }

    Future<T> getFuture() {
        return Future<T>(m_promise.get_future());
    }

private:
    std::promise<T> m_promise;
};

template<>
struct Promise<void> {
public:
    using value_type = void;

    Promise() = default;

    void setValue() {
        m_promise.set_value();
    }

    std::future<void> getFuture() {
        return m_promise.get_future();
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
        auto promise = std::make_shared<Promise<FinalU>>();
        auto nextFuture = promise->getFuture();

        SchedulerT::getInstance().schedule(
                [fut = m_future,
                 func = std::forward<Fn>(func),
                 prom = promise]() mutable {
                    if constexpr (std::is_void_v<T>) {
                        fut.get();
                        if constexpr (!Detail::IsFutureV<RawU>) {
                            if constexpr (std::is_void_v<RawU>) {
                                func();
                                prom->setValue();
                            } else {
                                FinalU result = func();
                                prom->setValue(std::move(result));
                            }
                        } else {
                            RawU innerFuture = func();
                            if constexpr (std::is_void_v<FinalU>) {
                                innerFuture.get();
                                prom->setValue();
                            } else {
                                FinalU inner = innerFuture.get();
                                prom->setValue(std::move(inner));
                            }
                        }
                    } else {
                        T value = fut.get();
                        if constexpr (!Detail::IsFutureV<RawU>) {
                            if constexpr (std::is_void_v<RawU>) {
                                func(value);
                                prom->setValue();
                            } else {
                                FinalU result = func(value);
                                prom->setValue(std::move(result));
                            }
                        } else {
                            RawU innerFuture = func(value);
                            if constexpr (std::is_void_v<FinalU>) {
                                innerFuture.get();
                                prom->setValue();
                            } else {
                                FinalU inner = innerFuture.get();
                                prom->setValue(std::move(inner));
                            }
                        }
                    }
                });
        return Future<FinalU, SchedulerT>(std::move(nextFuture));
    }

private:
    std::shared_future<T> m_future;
};

template<typename SchedulerT>
struct Future<void, SchedulerT> {
public:
    using value_type = void;

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
        auto promise = std::make_shared<Promise<FinalU>>();
        auto nextFuture = promise->getFuture();

        SchedulerT::getInstance().schedule(
                [fut = m_future,
                 func = std::forward<Fn>(func),
                 prom = promise]() mutable {
                    fut.get();
                    if constexpr (!Detail::IsFutureV<RawU>) {
                        if constexpr (std::is_void_v<RawU>) {
                            func();
                            prom->setValue();
                        } else {
                            FinalU res = func();
                            prom->setValue(std::move(res));
                        }
                    } else {
                        RawU innerFuture = func();
                        if constexpr (std::is_void_v<FinalU>) {
                            innerFuture.get();
                            prom->setValue();
                        } else {
                            FinalU inner = innerFuture.get();
                            prom->setValue(std::move(inner));
                        }
                    }
                });

        return Future<FinalU, SchedulerT>(std::move(nextFuture));
    }

private:
    std::shared_future<void> m_future;
};

MOE_END_NAMESPACE