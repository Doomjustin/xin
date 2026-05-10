export module xin.async.when_any;

import std;

import xin.async.io_context;
import xin.async.operation;
import xin.async.single_shot_operation;
import xin.async.sleep;
import xin.async.this_coroutine;
import xin.utility;


namespace detail {

template<typename First, typename... Rest>
inline constexpr bool all_same_v = (std::is_same_v<First, Rest> && ...);

} // namespace detail

export namespace xin::async {

/// @brief 等待多个 cancelable awaiter，任意一个完成即产生 winner。
///
/// - 首个完成者被记录为 winner；
/// - 其余已 arm 的 awaiter 会收到 cancel；
/// - 当所有分支都完成（包含 cancel 完成）后恢复上层协程。
///
/// 返回类型规则：
/// - 若所有 `resume_type` 相同：返回同一类型的 `std::expected<..., std::error_code>`；
/// - 否则：返回 `std::variant`，按 awaiter 索引区分 winner 分支。
template<cancelable_operation... Awaiters>
class WhenAnyAwaiter : public Operation {
public:
    using resume_type = std::conditional_t<
        detail::all_same_v<typename Awaiters::resume_type...>,
        std::expected<std::tuple_element_t<0, std::tuple<typename Awaiters::resume_type...>>,
                      std::error_code>,
        std::variant<std::expected<typename Awaiters::resume_type, std::error_code>...> >;

    explicit WhenAnyAwaiter(Awaiters&&... awaiters)
      : awaiters_{ std::forward<Awaiters>(awaiters)... }
    {
        setup_slots(std::index_sequence_for<Awaiters...>{});
    }

    [[nodiscard]]
    constexpr auto await_ready() const noexcept -> bool
    {
        return false;
    }

    /// @brief 挂起当前协程并 arm 全部分支。
    /// @return 若存在未完成分支则返回 `true`；否则返回 `false` 直接继续执行。
    auto await_suspend(std::coroutine_handle<> handle) noexcept -> bool
    {
        handle_ = handle;
        set_parents(std::index_sequence_for<Awaiters...>{});

        is_suspending_ = true;
        arm_all(std::index_sequence_for<Awaiters...>{});
        is_suspending_ = false;

        return pending_ > 0;
    }

    /// @brief 返回 winner 对应的结果。
    /// @return 同构类型时返回 `expected`；异构类型时返回 `variant<expected...>`。
    auto await_resume() -> resume_type
    {
        if constexpr (detail::all_same_v<typename Awaiters::resume_type...>)
            return get_winner_result(std::index_sequence_for<Awaiters...>{});
        else
            return collect_winner(std::index_sequence_for<Awaiters...>{});
    }

    /// @brief `WhenAnyAwaiter` 本体不直接接收完成回调。
    void complete(int /*result*/, std::uint32_t /*flags*/) noexcept override {}

    /// @brief 取消所有已 arm 分支。
    void cancel() noexcept
    {
        cancel_all(std::index_sequence_for<Awaiters...>{});
    }

    /// @brief 返回第一个 awaiter 绑定的 context。
    auto context() noexcept -> decltype(auto)
    {
        return std::get<0>(awaiters_).context();
    }

    /// @brief 获取 winner 索引。
    /// @return winner 下标；未决状态为 `-1`。
    [[nodiscard]]
    auto winner() const noexcept -> int
    {
        return winner_;
    }

private:
    /// @brief 每个 awaiter 对应的 parent slot。
    ///
    /// inner awaiter 完成后会先回调到 slot，再由 slot 转发给 owner。
    struct Slot : public Operation {
        WhenAnyAwaiter* owner{ nullptr };
        std::size_t index{ 0 };

        void complete(int result, std::uint32_t flags) noexcept override
        {
            owner->on_slot_complete(index, result, flags);
        }

        void cancel() noexcept
        {
            owner->cancel();
        }
    };

    std::tuple<Awaiters...> awaiters_;
    std::array<Slot, sizeof...(Awaiters)> slots_{};
    std::array<bool, sizeof...(Awaiters)> armed_{};
    std::coroutine_handle<> handle_;
    int pending_{ static_cast<int>(sizeof...(Awaiters)) };
    int winner_{ -1 };
    bool is_suspending_{ false };
    bool is_canceling_losers_{ false };

    /// @brief 初始化 slot 的 owner 与索引映射。
    template<std::size_t... Is>
    void setup_slots(std::index_sequence<Is...> /*index*/) noexcept
    {
        (..., (slots_[Is].owner = this, slots_[Is].index = Is));
    }

    /// @brief 将每个 inner awaiter 的 parent 指向对应 slot。
    template<std::size_t... Is>
    void set_parents(std::index_sequence<Is...> /*index*/) noexcept
    {
        (..., (std::get<Is>(awaiters_).parent = &slots_[Is]));
    }

    /// @brief arm 全部分支；若 winner 在挂起阶段已确定，立即取消其他分支。
    template<std::size_t... Is>
    void arm_all(std::index_sequence<Is...> /*index*/) noexcept
    {
        (..., arm_one<Is>());

        if (winner_ >= 0)
            cancel_losers(static_cast<std::size_t>(winner_),
                          std::index_sequence_for<Awaiters...>{});
    }

    /// @brief arm 单个分支，并在同步完成时更新 winner/pending。
    template<std::size_t I>
    void arm_one() noexcept
    {
        auto& awaiter = std::get<I>(awaiters_);
        if (awaiter.await_ready()) {
            if (winner_ < 0)
                winner_ = static_cast<int>(I);

            --pending_;
            return;
        }

        if (awaiter.await_suspend(handle_)) {
            armed_[I] = true;
            return;
        }

        if (winner_ < 0)
            winner_ = static_cast<int>(I);

        --pending_;
    }

    /// @brief slot 完成入口。
    ///
    /// 首个完成者会触发 loser cancel；所有分支收敛后恢复上层协程。
    void on_slot_complete(std::size_t index, int result, std::uint32_t flags) noexcept
    {
        bool is_first = false;
        if (winner_ < 0) {
            // First completion: record winner and cancel the other N-1 SQEs.
            winner_ = static_cast<int>(index);
            is_first = true;
            is_canceling_losers_ = true;
        }

        --pending_;

        if (is_first) {
            cancel_losers(index, std::index_sequence_for<Awaiters...>{});
            is_canceling_losers_ = false;
        }

        if (pending_ == 0 && !is_canceling_losers_ && !is_suspending_)
            this->resume(handle_, result, flags);
    }

    /// @brief 取消除 winner 外所有已 arm 分支。
    template<std::size_t... Is>
    void cancel_losers(std::size_t winner, std::index_sequence<Is...> /*index*/) noexcept
    {
        (..., (void)(Is != winner && armed_[Is] && (std::get<Is>(awaiters_).cancel(), true)));
    }

    /// @brief 取消所有已 arm 分支。
    template<std::size_t... Is>
    void cancel_all(std::index_sequence<Is...> /*index*/) noexcept
    {
        (..., (void)(armed_[Is] && (std::get<Is>(awaiters_).cancel(), true)));
    }

    /// @brief 在同构返回类型场景下提取 winner 结果。
    template<std::size_t I, std::size_t... Is>
    auto get_winner_result(std::index_sequence<I, Is...> /*index*/) -> resume_type
    {
        if (static_cast<std::size_t>(winner_) == I)
            return std::get<I>(awaiters_).await_resume();

        if constexpr (sizeof...(Is) > 0)
            return get_winner_result(std::index_sequence<Is...>{});

        // unreachable
        return std::get<0>(awaiters_).await_resume();
    }

    using variant_type =
        std::variant<std::expected<typename Awaiters::resume_type, std::error_code>...>;

    /// @brief 在异构返回类型场景下收集 winner 结果到 variant。
    template<std::size_t I, std::size_t... Is>
    auto collect_winner(std::index_sequence<I, Is...> /*index*/) -> variant_type
    {
        if (static_cast<std::size_t>(winner_) == I)
            return variant_type{ std::in_place_index<I>, std::get<I>(awaiters_).await_resume() };

        if constexpr (sizeof...(Is) > 0)
            return collect_winner(std::index_sequence<Is...>{});

        return variant_type{ std::in_place_index<0>, std::get<0>(awaiters_).await_resume() };
    }
};

/// @brief 创建 `WhenAnyAwaiter`。
/// @param[in] awaiters 参与竞速的 cancelable awaiter 列表。
/// @return `WhenAnyAwaiter`。
template<cancelable_operation... Awaiters>
auto when_any(Awaiters&&... awaiters)
{
    return WhenAnyAwaiter<std::remove_cvref_t<Awaiters>...>{ std::forward<Awaiters>(awaiters)... };
}

} // namespace xin::async