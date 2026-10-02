#include "serenkai/base/panic.hpp" // IWYU pragma: keep

#ifdef NDEBUG
#define SE_ASSERT(expr) ((void)0)
#else
#define SE_ASSERT(expr)                                                        \
    do {                                                                       \
        if (!(expr)) [[unlikely]] {                                            \
            ::serenkai::panic_at("assertion failed: " #expr,                   \
                                 std::source_location::current());             \
        }                                                                      \
    } while (false)
#endif

#define SE_VERIFY(expr)                                                        \
    do {                                                                       \
        if (!(expr)) [[unlikely]] {                                            \
            ::serenkai::panic_at("verification failed: " #expr,                \
                                 std::source_location::current());             \
        }                                                                      \
    } while (false)
