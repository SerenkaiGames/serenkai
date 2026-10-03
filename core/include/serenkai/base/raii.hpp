#pragma once

#include <concepts>
#include <utility>

namespace serenkai {
template <typename F>
concept Initializer = std::invocable<F>;

template <typename F>
concept Cleaner = std::invocable<F>;

/// @brief An RAII guard class
///
/// Calls InitFn on construction
/// Calls CleanupFn on destruction
template <Initializer InitFn, Cleaner CleanupFn> class RaiiGuard {
public:
    RaiiGuard(InitFn init, CleanupFn cleanup) : m_cleanup(std::move(cleanup)) {
        init();
        m_active = true;
    }

    ~RaiiGuard() { reset(); }

    RaiiGuard(const RaiiGuard&) = delete;
    RaiiGuard& operator=(const RaiiGuard&) = delete;

    RaiiGuard(RaiiGuard&& o) noexcept
        : m_cleanup(std::move(o.m_cleanup)),
          m_active(std::exchange(o.m_active, false)) {}

    RaiiGuard& operator=(RaiiGuard&& o) noexcept {
        if (this != &o) {
            reset();
            m_cleanup = std::move(o.m_cleanup);
            m_active = std::exchange(o.m_active, false);
        }
        return *this;
    }

    void reset() {
        if (m_active) {
            m_cleanup();
            m_active = false;
        }
    }

    void release() noexcept { m_active = false; }

private:
    CleanupFn m_cleanup;
    bool m_active{false};
};

template <Initializer InitFn, Cleaner CleanupFn>
RaiiGuard(InitFn, CleanupFn) -> RaiiGuard<InitFn, CleanupFn>;

} // namespace serenkai
