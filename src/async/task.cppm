export module xin.async.task;

import std;

export namespace xin::async {

/// @brief 任务结束时的对称转移 awaiter。
///
/// 若存在 continuation（`promise.next`），最终挂起会直接转移到该 continuation；
/// 否则转移到 `std::noop_coroutine()`。
class FinalAwaiter {
public:
    [[nodiscard]]
    constexpr auto await_ready() const noexcept -> bool
    {
        return false;
    }

    template<typename Promise>
    auto await_suspend(std::coroutine_handle<Promise> handle) const noexcept
        -> std::coroutine_handle<>
    {
        auto next = handle.promise().next;
        return next ? next : std::noop_coroutine();
    }

    void await_resume() const noexcept {}
};

/// @brief `Task` 的 co_await 适配器。
/// @tparam Promise 对应协程的 promise 类型。
template<typename Promise>
class Awaiter {
public:
    using handle_type = std::coroutine_handle<Promise>;

    explicit Awaiter(handle_type handle)
      : handle_{ handle }
    {}

    [[nodiscard]]
    auto await_ready() const noexcept -> bool
    {
        return !handle_ || handle_.done();
    }

    auto await_suspend(std::coroutine_handle<> next) -> std::coroutine_handle<>
    {
        handle_.promise().next = next;
        return handle_;
    }

    auto await_resume() const -> decltype(auto)
    {
        if (!handle_)
            throw std::logic_error{ "Invalid coroutine handle" };

        return handle_.promise().result();
    }

private:
    handle_type handle_;
};

/// @brief Task promise 的公共基类，承载 continuation 和异常状态。
struct PromiseBase {
    std::coroutine_handle<> next;
    std::exception_ptr exception_;

    auto initial_suspend() noexcept -> std::suspend_always
    {
        return {};
    }

    auto final_suspend() noexcept -> FinalAwaiter
    {
        return {};
    }

    void unhandled_exception() noexcept
    {
        exception_ = std::current_exception();
    }
};

template<typename T = void>
class Task;

/// @brief `Task<T>` 的 promise 类型。
/// @tparam T 任务返回值类型。
template<typename T>
struct Promise : PromiseBase {
    std::optional<T> value_;

    auto get_return_object() noexcept -> Task<T>;

    template<typename U>
        requires std::convertible_to<U&&, T>
    void return_value(U&& value) noexcept(std::is_nothrow_constructible_v<T, U&&>)
    {
        value_.emplace(std::forward<U>(value));
    }

    [[nodiscard]]
    auto result() -> T
    {
        if (exception_)
            std::rethrow_exception(exception_);

        if (!value_)
            throw std::logic_error{ "No value returned from coroutine" };

        auto out = std::move(*value_);
        value_.reset();
        return out;
    }
};

/// @brief `Task<void>` 的 promise 特化。
template<>
struct Promise<void> : PromiseBase {
    auto get_return_object() noexcept -> Task<void>;

    void return_void() noexcept {}

    void result()
    {
        if (exception_)
            std::rethrow_exception(exception_);
    }
};

/// @brief 可 co_await 的轻量任务类型。
///
/// `Task` 在创建后默认处于 `initial_suspend`，需要通过 `co_await` 或手动 `resume()` 执行。
///
/// @tparam T 任务结果类型，`void` 表示无返回值。
template<typename T>
class Task {
public:
    using promise_type = Promise<T>;
    using handle_type = std::coroutine_handle<promise_type>;

    Task() = default;

    explicit Task(handle_type handle)
      : handle_{ handle }
    {}

    Task(const Task&) = delete;
    auto operator=(const Task&) -> Task& = delete;

    Task(Task&& other) noexcept
      : handle_{ std::exchange(other.handle_, {}) }
    {}

    auto operator=(Task&& other) noexcept -> Task&
    {
        if (this == &other)
            return *this;

        if (handle_)
            handle_.destroy();

        handle_ = std::exchange(other.handle_, nullptr);
        return *this;
    }

    ~Task()
    {
        if (handle_)
            handle_.destroy();
    }

    [[nodiscard]]
    auto done() const noexcept -> bool
    {
        return !handle_ || handle_.done();
    }

    [[nodiscard]]
    auto handle() const noexcept -> handle_type
    {
        return handle_;
    }

    auto operator co_await() && noexcept
    {
        return Awaiter<promise_type>{ handle_ };
    }

private:
    handle_type handle_;
};

template<typename T>
auto Promise<T>::get_return_object() noexcept -> Task<T>
{
    return Task<T>{ std::coroutine_handle<Promise<T>>::from_promise(*this) };
}

auto Promise<void>::get_return_object() noexcept -> Task<void>
{
    return Task<void>{ std::coroutine_handle<Promise<void>>::from_promise(*this) };
}

} // namespace xin::async