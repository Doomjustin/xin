export module xin.async.this_coroutine;

import std;

import xin.async.io_context;


namespace detail {

unsigned entries = 1024;

thread_local xin::async::IOContext* bound_context = nullptr;

} // namespace detail

export namespace xin::async::this_coroutine {

auto context() -> IOContext&
{
    if (detail::bound_context)
        return *detail::bound_context;

    // 没有绑定的上下文，创建一个线程专用的IOContext并绑定
    // 但是这个context不会被实际使用，在context::run的时候，会被ContextBinder替换掉
    thread_local auto ctx = std::make_unique<IOContext>(detail::entries);
    return *ctx;
}

auto setup_buffer_ring(unsigned entries, unsigned size) -> unsigned
{
    return context().setup_buffer_ring(entries, size);
}

void setup_entries(unsigned new_entries)
{
    detail::entries = new_entries;
}

class ContextBinder {
public:
    ContextBinder() noexcept
    {
        thread_local auto ctx = std::make_unique<IOContext>(detail::entries);
        context_ = ctx.get();
        previous_ = detail::bound_context;
        detail::bound_context = context_;
    }

    explicit ContextBinder(IOContext& ctx) noexcept
      : context_{ &ctx }
      , previous_{ detail::bound_context }
    {
        detail::bound_context = &ctx;
    }

    ContextBinder(const ContextBinder&) = delete;
    auto operator=(const ContextBinder&) -> ContextBinder& = delete;

    ~ContextBinder() noexcept
    {
        detail::bound_context = previous_;
    }

    [[nodiscard]] auto context() noexcept -> IOContext&
    {
        return *context_;
    }

private:
    IOContext* context_;
    IOContext* previous_;
};

} // namespace xin::async::this_coroutine
