export module xin.async;

/// @brief `xin::async` 聚合导出模块。
///
/// 该模块集中导出常用 async 组件，便于外部通过单一 import 使用。

export import xin.async.awaiter;
export import xin.async.channel;
export import xin.async.co_spawn;
export import xin.async.io_context;
export import xin.async.poll_awaiter;
export import xin.async.run;
export import xin.async.shift_to;
export import xin.async.signal;
export import xin.async.single_shot_awaiter;
export import xin.async.sleep;
export import xin.async.stoppable_promise;
export import xin.async.task;
export import xin.async.this_coro;
export import xin.async.timer_awaiter;
