export module xin.async.run;

import std;

import xin.async.co_spawn;
import xin.async.io_context;


export namespace xin::async {

/// @brief 在单个 IOContext 上运行一个 awaitable 工厂。
/// @tparam F 可调用对象类型。
/// @tparam Args 调用参数类型列表。
/// @param[in] func 返回 awaitable 的可调用对象。
/// @param[in] args 传递给 `func` 的参数。
/// @return 无。
template<typename F, typename... Args>
void run(F&& func, Args&&... args)
{
    IOContext context;
    co_spawn(context, std::invoke(std::forward<F>(func), std::forward<Args>(args)...));
    context.run();
}

/// @brief 在单个 IOContext 上运行 awaitable 工厂，并注入外部 stop_token。
/// @tparam F 可调用对象类型。
/// @tparam Args 调用参数类型列表。
/// @param[in] token 停止请求来源 token。
/// @param[in] func 返回 awaitable 的可调用对象。
/// @param[in] args 传递给 `func` 的参数。
/// @return 无。
template<typename F, typename... Args>
void run(std::stop_token token, F&& func, Args&&... args)
{
    IOContext context;
    co_spawn(context, token, std::invoke(std::forward<F>(func), std::forward<Args>(args)...));
    context.run();
}

/// @brief 在多个线程上并行运行 awaitable 工厂。
/// @tparam F 可调用对象类型。
/// @tparam Args 调用参数类型列表。
/// @param[in] thread_count 线程数量，主线程也会运行一个 IOContext。
/// @param[in] func 返回 awaitable 的可调用对象。
/// @param[in] args 传递给 `func` 的参数。
/// @return 无。
template<typename F, typename... Args>
void run(std::integral auto thread_count, F&& func, Args&&... args)
{
    std::vector<std::jthread> threads;

    for (int i = 1; i < thread_count; ++i) {
        threads.emplace_back([func, args...] mutable {
            IOContext context;
            co_spawn(context, std::invoke(std::forward<F>(func), std::forward<Args>(args)...));
            context.run();
        });
    }

    run(std::forward<F>(func), std::forward<Args>(args)...);
}

/// @brief 在多个线程上并行运行 awaitable 工厂，并注入 stop_token。
/// @tparam F 可调用对象类型。
/// @tparam Args 调用参数类型列表。
/// @param[in] thread_count 线程数量，主线程也会运行一个 IOContext。
/// @param[in] token 停止请求来源 token。
/// @param[in] func 返回 awaitable 的可调用对象。
/// @param[in] args 传递给 `func` 的参数。
/// @return 无。
template<typename F, typename... Args>
void run(std::integral auto thread_count, std::stop_token token, F&& func, Args&&... args)
{
    std::vector<std::jthread> threads;

    for (int i = 1; i < thread_count; ++i) {
        threads.emplace_back([token, func, args...] mutable {
            IOContext context;
            co_spawn(context, token,
                     std::invoke(std::forward<F>(func), std::forward<Args>(args)...));
            context.run();
        });
    }

    run(token, std::forward<F>(func), std::forward<Args>(args)...);
}

} // namespace xin::async