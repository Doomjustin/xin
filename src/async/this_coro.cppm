export module xin.async.this_coro;

import std;

export namespace xin::async::this_coro {

struct context_tag {};

struct stop_token_tag {};

constexpr context_tag context{};

constexpr stop_token_tag stop_token{};

} // namespace xin::async::this_coro