#pragma once

#include <type_traits>
#include <utility>
namespace serenkai {
namespace detail {
template <typename Class, typename Ret, typename Subsystem, typename... Args>
auto wrap_callable_impl(auto get_subsystem, auto callable,
                        Ret (Class::*)(Subsystem*, Args...) const) {
    return [get_subsystem, callable](Args... args) -> Ret {
        if (auto* sub = get_subsystem()) {
            return callable(sub, std::forward<Args>(args)...);
        }
        if constexpr (!std::is_void_v<Ret>) {
            return Ret{};
        }
    };
}

template <typename Class, typename Ret, typename Subsystem, typename... Args>
auto wrap_callable_impl(auto get_subsystem, auto callable,
                        Ret (Class::*)(Subsystem*, Args...)) {
    return [get_subsystem, callable](Args... args) -> Ret {
        if (auto* sub = get_subsystem()) {
            return callable(sub, std::forward<Args>(args)...);
        }
        if constexpr (!std::is_void_v<Ret>) {
            return Ret{};
        }
    };
}

} // namespace detail

template <typename Subsystem, typename Ret, typename... Args>
auto wrap_facade(auto get_subsystem, Ret (Subsystem::*mem_fn)(Args...)) {
    return [get_subsystem, mem_fn](Args... args) -> Ret {
        if (auto* sub = get_subsystem()) {
            return (sub->*mem_fn)(std::forward<Args>(args)...);
        }
        if constexpr (!std::is_void_v<Ret>) {
            return Ret{};
        }
    };
}

template <typename Subsystem, typename Ret, typename... Args>
auto wrap_facade(auto get_subsystem, Ret (Subsystem::*mem_fn)(Args...) const) {
    return [get_subsystem, mem_fn](Args... args) -> Ret {
        if (auto* sub = get_subsystem()) {
            return (sub->*mem_fn)(std::forward<Args>(args)...);
        }
        if constexpr (!std::is_void_v<Ret>) {
            return Ret{};
        }
    };
}

template <typename F>
    requires requires { &std::decay_t<F>::operator(); }
auto wrap_facade(auto get_subsystem, F callable) {
    return detail::wrap_callable_impl(get_subsystem, callable,
                                      &std::decay_t<F>::operator());
}

} // namespace serenkai