export module xin.async.post;

import std;

import xin.async.io_context;
import xin.async.operation;


export namespace xin::async {

/// @brief 用于把普通函数对象投递到 `IOContext` 中执行的 operation。
///
/// 该对象强制堆分配，并在 `complete()` 中执行回调后自删除，
/// 以保证回调执行期间对象生命周期始终有效。
template<typename Func>
class DispatchOperation : public Operation {
public:
    /// @brief 构造投递操作。
    /// @param[in] context 目标执行上下文。
    /// @param[in] f 待执行回调。
    DispatchOperation(IOContext& context, Func&& f)
      : context_{ &context }
      , func_{ std::forward<Func>(f) }
    {}

    /// @brief 完成投递并执行回调。
    ///
    /// 这里会先减少 work 计数，再调用回调，最后释放自身。
    void complete(int result, unsigned flags) noexcept override
    {
        context_->drop_work();
        func_();
        delete this;
    }

private:
    IOContext* context_;
    Func func_;
};

/// @brief 将回调投递到指定 `IOContext` 的队列中执行。
///
/// 该版本始终走跨线程投递路径，适合从任意线程提交。
/// @param[in] context 目标执行上下文。
/// @param[in] f 待执行回调。
template<typename Func>
void post(IOContext& context, Func&& f)
{
    context.add_work();
    auto* op = new DispatchOperation<Func>{ context, std::forward<Func>(f) };
    context.post(op);
}

/// @brief 将回调派发到指定 `IOContext`。
///
/// 若当前线程就是 owner thread，则直接 `submit()`；否则退化为跨线程 `post()`。
/// @param[in] context 目标执行上下文。
/// @param[in] func 待执行回调。
template<typename Func>
void dispatch(IOContext& context, Func&& func)
{
    context.add_work();
    auto* op = new DispatchOperation<Func>{ context, std::forward<Func>(func) };
    if (context.is_owner_thread())
        context.submit(op);
    else
        context.post(op);
}

} // namespace xin::async