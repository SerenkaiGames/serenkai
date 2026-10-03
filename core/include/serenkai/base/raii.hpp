#pragma once

#include <concepts>
#include <utility>

namespace serenkai {

/// @brief An RAII guard class
///
/// Calls InitFn on construction
/// Calls CleanupFn on destruction
template <std::invocable InitFn, std::invocable CleanupFn> class RaiiGuard {
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

template <std::invocable InitFn, std::invocable CleanupFn>
RaiiGuard(InitFn, CleanupFn) -> RaiiGuard<InitFn, CleanupFn>;

template <typename F, typename T, typename... Args>
concept Initializer = std::invocable<F, T&, Args...>;

template <typename F, typename T>
concept Cleaner = std::invocable<F, T&>;

template <typename T, Initializer<T> InitFn, Cleaner<T> CleanupFn>
class RaiiWrapper {
public:
    template <typename... Args>
        requires Initializer<InitFn, T, Args...>
    RaiiWrapper(InitFn init, CleanupFn cleanup, Args&&... args)
        : m_init(std::move(init)), m_cleanup(std::move(cleanup)) {
        m_init(m_value, std::forward<Args>(args)...);
        m_active = true;
    }

    ~RaiiWrapper() { reset(); }

    RaiiWrapper(const RaiiWrapper&) = delete;
    RaiiWrapper& operator=(const RaiiWrapper&) = delete;

    RaiiWrapper(RaiiWrapper&& o) noexcept
        : m_value(std::move(o.m_value)), m_init(std::move(o.m_init)),
          m_cleanup(std::move(o.m_cleanup)),
          m_active(std::exchange(o.m_active, false)) {}

    RaiiWrapper& operator=(RaiiWrapper&& o) noexcept {
        if (this != &o) {
            reset();
            m_value = std::move(o.m_value);
            m_init = std::move(o.m_init);
            m_cleanup = std::move(o.m_cleanup);
            m_active = std::exchange(o.m_active, false);
        }
        return *this;
    }

    void reset() {
        if (m_active) {
            m_cleanup(m_value);
            m_active = false;
        }
    }
    void release() noexcept { m_active = false; }

    T& get() noexcept { return m_value; }
    const T& get() const noexcept { return m_value; }
    T* operator->() noexcept { return &m_value; }
    const T* operator->() const noexcept { return &m_value; }
    T& operator*() noexcept { return m_value; }
    const T& operator*() const noexcept { return &m_value; }
    explicit operator bool() const noexcept { return m_active; }

private:
    T m_value{};
    InitFn m_init;
    CleanupFn m_cleanup;
    bool m_active{false};
};

template <typename T, typename InitFn, typename CleanupFn, typename... Args>
    requires Initializer<InitFn, T, Args...> && Cleaner<CleanupFn, T>
auto make_raii(InitFn init, CleanupFn cleanup, Args&&... args) {
    return RaiiWrapper<T, InitFn, CleanupFn>(
        std::move(init), std::move(cleanup), std::forward<Args>(args)...);
}

} // namespace serenkai
