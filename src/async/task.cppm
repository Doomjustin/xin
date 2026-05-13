module;

#include <cassert>

export module xin.async.task;

import std;

import xin.async.awaiter;
import xin.async.io_context;
import xin.async.this_coro;
import xin.utility;


namespace detail {

/// @brief Task 的非 void 返回值存储策略。
/// @tparam T 任务返回类型。
template<typename T>
class TaskReturnType {
protected:
    std::optional<T> result_;

public:
    /// @brief 保存 coroutine 返回值。
    /// @param[in] value 要写入的返回值。
    template<typename U>
        requires std::convertible_to<U&&, T>
    void return_value(U&& value) noexcept(std::is_nothrow_constructible_v<T, U&&>)
    {
        result_.emplace(std::forward<U>(value));
    }

    /// @brief 取出已保存的返回值。
    /// @return 以右值形式返回保存的结果。
    auto result() noexcept -> T&&
    {
        return std::move(*result_);
    }
};

/// @brief Task<void> 的返回值策略特化。
template<>
class TaskReturnType<void> {
public:
    /// @brief 处理 void 返回。
    void return_void() noexcept {}
};

} // namespace detail

export namespace xin::async {

/// @brief 支持 co_await 的可组合任务类型。
///
/// Task 采用 lazy 启动语义：创建后不会立刻执行，只有被 co_await 或
/// 作为 coroutine 返回对象继续驱动时才会运行。
/// @tparam T 任务结果类型，默认为 void。
template<typename T = void>
class Task {
public:
    struct promise_type;
    using handle_type = std::coroutine_handle<promise_type>;

private:
    handle_type handle_;

public:
    /// @brief Task 对应的 coroutine promise 类型。
    struct promise_type : detail::TaskReturnType<T> {
        std::coroutine_handle<> next;
        std::stop_token stop_token;
        IOContext* context{ nullptr };

        union {
            std::exception_ptr exception;
        };
        bool has_exception{ false };

        /// @brief 默认构造 promise。
        promise_type() {}

        /// @brief 析构 promise 并释放异常对象。
        ~promise_type()
        {
            if (has_exception)
                exception.~exception_ptr();
        }

        /// @brief 构造 Task 返回对象。
        /// @return 与当前 promise 绑定的 Task。
        auto get_return_object() noexcept -> Task
        {
            return Task{ handle_type::from_promise(*this) };
        }

        /// @brief 初始挂起点，Task 采用 lazy 启动。
        /// @return 总是 suspend_always。
        auto initial_suspend() noexcept -> std::suspend_always
        {
            return {};
        }

        /// @brief 最终挂起点，将控制权返回给父 coroutine。
        /// @return 一个在 await_suspend 中恢复 next 的 awaiter。
        auto final_suspend() noexcept
        {
            struct Awaiter {
                promise_type* promise;

                constexpr auto await_ready() const noexcept -> bool
                {
                    return false;
                }

                auto await_suspend(std::coroutine_handle<> handle) noexcept
                {
                    return promise->next ? promise->next : std::noop_coroutine();
                }

                void await_resume() noexcept {}
            };

            return Awaiter{ this };
        }

        /// @brief 捕获未处理异常。
        void unhandled_exception() noexcept
        {
            new (&exception) std::exception_ptr(std::current_exception());
            has_exception = true;
        }

        /// @brief 为基于 Awaiter 的对象接入 stop_token 取消传播。
        /// @tparam Awaitable 被包装的 awaitable 类型。
        template<typename Awaitable>
        class StopTokenWrapper {
        private:
            struct CancelFn {
                StopTokenWrapper<Awaitable>* wrapper;

                /// @brief stop 回调：向 IOContext 提交取消请求。
                void operator()()
                {
                    if (wrapper) {
                        wrapper->context().cancel(wrapper->inner_);
                        wrapper->is_cancelled_ = true;
                    }
                }
            };

            Awaitable inner_;
            promise_type* promise_;
            bool is_cancelled_{ false };
            std::optional<std::stop_callback<CancelFn>> stop_callback_;

        public:
            /// @brief 构造取消包装器。
            /// @param[in] awaitable 被包装的 awaitable。
            /// @param[in] promise 当前 Task 的 promise。
            StopTokenWrapper(Awaitable awaitable, promise_type* promise)
              : inner_{ std::move(awaitable) }
              , promise_{ promise }
            {}

            /// @brief 析构时注销 stop_callback。
            ~StopTokenWrapper()
            {
                stop_callback_.reset();
            }

            /// @brief 访问当前 Task 绑定的 IOContext。
            /// @return IOContext 引用。
            auto context() const noexcept -> IOContext&
            {
                return *promise_->context;
            }

            /// @brief 该包装器总是需要进入 await_suspend 以安装 stop_callback。
            auto await_ready() const noexcept -> bool
            {
                return false;
            }

            /// @brief 安装 stop_callback 并委托给 inner await_suspend。
            /// @param[in] handle 当前 coroutine 句柄。
            /// @return inner 返回的下一个恢复句柄。
            auto await_suspend(std::coroutine_handle<> handle) noexcept -> std::coroutine_handle<>
            {
                if (promise_->stop_token.stop_possible())
                    stop_callback_.emplace(promise_->stop_token, CancelFn{ this });

                return inner_.await_suspend(handle, context());
            }

            /// @brief 返回 inner 的结果，若已取消则返回 operation_canceled。
            /// @return inner 的 await_resume 结果类型。
            auto await_resume() noexcept
            {
                using ResultType = decltype(inner_.await_resume());

                if (is_cancelled_)
                    return ResultType{ std::unexpect,
                                       std::make_error_code(std::errc::operation_canceled) };

                return ResultType{ inner_.await_resume() };
            }
        };

        /// @brief 根据 awaitable 类型做 coroutine 环境注入与适配。
        ///
        /// 对 `this_coro` 标签对象，直接返回当前 promise 持有的环境对象，
        /// 并采用 `await_ready() == true` 的立即完成语义，避免无恢复方的空挂起。
        /// 对派生自 `Awaiter` 的对象，包装为 StopTokenWrapper 以接入取消传播。
        template<typename Awaitable>
        auto await_transform(Awaitable&& awaitable)
        {
            using Tag = std::remove_cvref_t<Awaitable>;

            if constexpr (std::same_as<Tag, this_coro::context_tag>) {
                struct Awaiter {
                    IOContext* io_context;

                    /// @brief 读取 context 为同步动作，不应触发挂起。
                    constexpr auto await_ready() const noexcept -> bool
                    {
                        return true;
                    }

                    void await_suspend(std::coroutine_handle<> handle) noexcept {}

                    auto await_resume() const noexcept -> IOContext*
                    {
                        return io_context;
                    }
                };

                return Awaiter{ context };
            }
            else if constexpr (std::same_as<Tag, this_coro::stop_token_tag>) {
                struct Awaiter {
                    std::stop_token token;

                    /// @brief 读取 stop_token 为同步动作，不应触发挂起。
                    constexpr auto await_ready() const noexcept -> bool
                    {
                        return true;
                    }

                    void await_suspend(std::coroutine_handle<> handle) noexcept {}

                    auto await_resume() const noexcept -> std::stop_token
                    {
                        return token;
                    }
                };

                return Awaiter{ stop_token };
            }
            else if constexpr (std::derived_from<Tag, Awaiter>) {
                return StopTokenWrapper<Awaitable>{ std::forward<Awaitable>(awaitable), this };
            }
            else {
                return std::forward<Awaitable>(awaitable);
            }
        }
    };

    Task() = default;

    /// @brief 使用 coroutine handle 构造 Task。
    /// @param[in] handle 与 promise 绑定的句柄。
    Task(handle_type handle)
      : handle_{ handle }
    {}

    Task(const Task&) = delete;
    auto operator=(const Task&) -> Task& = delete;

    /// @brief 移动构造，转移 coroutine 所有权。
    /// @param[in] other 源 Task。
    Task(Task&& other) noexcept
      : handle_{ std::exchange(other.handle_, {}) }
    {}

    /// @brief 移动赋值，释放旧句柄并接管新句柄。
    /// @param[in] other 源 Task。
    /// @return 当前对象引用。
    auto operator=(Task&& other) noexcept -> Task&
    {
        if (this == &other)
            return *this;

        if (handle_)
            handle_.destroy();

        handle_ = std::exchange(other.handle_, {});
        return *this;
    }

    /// @brief 析构 Task，销毁仍持有的 coroutine frame。
    ~Task()
    {
        if (handle_)
            handle_.destroy();
    }

    /// @brief Task 作为 awaitable 时总是先挂起，由 await_suspend 驱动。
    constexpr auto await_ready() const noexcept -> bool
    {
        return false;
    }

    /// @brief 挂起父 coroutine，连接 continuation 并传播执行环境。
    /// @param[in] parent 父 coroutine 句柄。
    /// @return 子 Task 对应的 coroutine 句柄。
    template<typename Promise>
    auto await_suspend(std::coroutine_handle<Promise> parent) -> std::coroutine_handle<>
    {
        handle_.promise().next = parent;

        // 如果父协程中有 context，则将其传递给子协程
        if constexpr (requires { parent.promise().context; })
            handle_.promise().context = parent.promise().context;

        // 如果父协程中有 stop_token，则将其传递给子协程
        if constexpr (requires { parent.promise().stop_token; })
            handle_.promise().stop_token = parent.promise().stop_token;

        return handle_;
    }

    /// @brief 恢复后返回 Task 结果或重新抛出捕获异常。
    /// @return 当 T 非 void 时返回结果；当 T 为 void 时无返回值。
    auto await_resume() const
    {
        if (handle_.promise().has_exception)
            std::rethrow_exception(handle_.promise().exception);

        if constexpr (!std::is_void_v<T>)
            return handle_.promise().result();
        else
            return;
    }

    /// @brief 获取底层 coroutine handle。
    /// @return 当前持有的句柄。
    auto handle() const noexcept
    {
        return handle_;
    }

    /// @brief 放弃句柄所有权，不销毁 coroutine frame。
    void release() noexcept
    {
        handle_ = nullptr;
    }
};

} // namespace xin::async