export module xin.async.all;

import std;

import xin.async.awaitable;
import xin.async.task;
import xin.async.task_group;


export namespace xin::async {

/// @brief 并发运行多个 awaitable，并等待全部完成。
///
/// `all(...)` 会把每个 awaitable 交给 `TaskGroup` 管理，
/// 然后在 `join()` 处等待所有任务收敛。
///
/// @tparam Awaitables 满足 `awaitable` concept 的类型集合。
/// @param[in] awaitables 待并发执行的 awaitable 列表。
/// @return 在所有 awaitable 完成后结束的 `Task<>`。
template<awaitable... Awaitables>
auto all(Awaitables&&... awaitables) -> Task<>
{
    static_assert(sizeof...(Awaitables) > 0, "all requires at least one awaitable");

    TaskGroup group;
    (group.spawn(std::forward<Awaitables>(awaitables)), ...);
    co_await group.join();
}

} // namespace xin::async