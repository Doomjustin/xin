export module xin.async.any;

import std;

import xin.async.awaitable;
import xin.async.task;
import xin.async.task_group;


export namespace xin::async {

/// @brief 将可调用对象包装为 `stop_token` 在末尾参数的 provider。
///
/// 生成的 provider 统一签名为 `provider(std::stop_token)`，便于交给 `any(...)`。
/// @tparam F 可调用对象类型。
/// @tparam Args 预绑定参数类型。
/// @param[in] f 原始可调用对象。
/// @param[in] args 预绑定参数。
/// @return 可接收 `std::stop_token` 的 provider。
template<typename F, typename... Args>
    requires std::invocable<F, std::decay_t<Args>&..., std::stop_token>
auto task(F&& f, Args&&... args)
{
    return [f = std::forward<F>(f), tup = std::tuple<std::decay_t<Args>...>(std::forward<Args>(
                                        args)...)](std::stop_token token) mutable {
        return std::apply([&](auto&... a) { return f(a..., std::move(token)); }, tup);
    };
}

/// @brief 将可调用对象包装为 `stop_token` 在首参数的 provider。
///
/// 当函数同时可匹配“token 在末尾”时，本重载会被排除，避免二义性。
/// @tparam F 可调用对象类型。
/// @tparam Args 预绑定参数类型。
/// @param[in] f 原始可调用对象。
/// @param[in] args 预绑定参数。
/// @return 可接收 `std::stop_token` 的 provider。
template<typename F, typename... Args>
    requires std::invocable<F, std::stop_token, std::decay_t<Args>&...> &&
             (!std::invocable<F, std::decay_t<Args>&..., std::stop_token>)
auto task(F&& f, Args&&... args)
{
    return [f = std::forward<F>(f), tup = std::tuple<std::decay_t<Args>...>(std::forward<Args>(
                                        args)...)](std::stop_token token) mutable {
        return std::apply([&](auto&... a) { return f(std::move(token), a...); }, tup);
    };
}

/// @brief 约束：可接收 `std::stop_token` 并返回 awaitable 的 provider。
template<typename Provider>
concept stop_awaitable_provider =
    std::invocable<std::decay_t<Provider>, std::stop_token> &&
    awaitable<std::invoke_result_t<std::decay_t<Provider>, std::stop_token>>;

/// @brief 单个 provider 的运行包装。
///
/// provider 完成后会请求停止 `TaskGroup`，使 `any(...)` 具备“先完成先返回”语义。
template<stop_awaitable_provider Provider>
auto any_spawned_task(Provider provider, TaskGroup& group) -> Task<>
{
    co_await std::move(provider)(group.stop_token());
    group.request_stop();
}

/// @brief 并发运行多个 provider，任意一个完成后停止其余任务。
///
/// @tparam Providers 满足 `stop_awaitable_provider` 的 provider 类型集合。
/// @param[in] providers 待并发运行的 provider 列表。
/// @return 在“首个 provider 完成且其余任务停止收敛”后完成的 `Task<>`。
template<stop_awaitable_provider... Providers>
auto any(Providers&&... providers) -> Task<>
{
    static_assert(sizeof...(Providers) > 0, "any requires at least one provider");

    TaskGroup group;
    (group.spawn(any_spawned_task(std::forward<Providers>(providers), group)), ...);
    co_await group.join();
}

} // namespace xin::async