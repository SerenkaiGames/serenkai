#pragma once

#include <type_traits>

namespace serenkai {
template <class T>
concept PlainType =
    !std::is_reference_v<T> && !std::is_pointer_v<T> && !std::is_const_v<T>;
}
