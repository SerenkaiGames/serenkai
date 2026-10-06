#pragma once

#include "serenkai/base/math.hpp"

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>

namespace serenkai {

namespace res_detail {
constexpr bool is_valid_ns(char c) noexcept {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
           (c >= '0' && c <= '9') || c == '_' || c == '-';
}

constexpr bool is_valid_path(char c) noexcept {
    return is_valid_ns(c) || c == '.' || c == ':' || c == '/';
}

constexpr std::size_t fnv1a(std::string_view sv) noexcept {
    std::size_t h = 0xcbf29ce484222325ULL;
    for (char c : sv) {
        h ^= static_cast<unsigned char>(c);
        h *= 0x100000001b3ULL;
    }
    return h;
}
constexpr std::size_t hash_of(std::string_view ns,
                              std::string_view path) noexcept {
    return combine32(hash_to_32(fnv1a(ns)), hash_to_32(fnv1a(path)));
}

} // namespace res_detail

/// @brief Defines a struct for the name and path of a resource file
///
/// Uses a namespace-based approach, consisting of namespace and path; path uses
/// '/' For example, a resource path can be example:texture/example.png
/// This type is also used to identify in-game events and similar objects, for
/// example example:events/example_event
/// @note If no namespace is specified, the default is serenkai
/// @note Only uppercase letters, lowercase letters, digits, and the characters
/// '.', '_', ':', '/', '-' are allowed.
/// @note Maximum total length is CAPACITY (128 characters).
struct ResourceLocation {
    // Maximum string length.
    static constexpr std::size_t CAPACITY = 128;
    static constexpr std::string_view DEFAULT_NAMESPACE = "serenkai";

    std::size_t hash = 0;
    char buf[CAPACITY + 1]{};
    std::uint8_t len = 0;
    std::uint8_t ns_len = 0;

    constexpr ResourceLocation() noexcept = default;

    explicit constexpr ResourceLocation(std::string_view full) noexcept {
        std::string_view ns;
        std::string_view path;
        if (!validate(full, ns, path)) {
            return;
        }

        std::size_t w = 0;
        for (char c : ns) {
            buf[w++] = c;
        }

        ns_len = static_cast<std::uint8_t>(w);
        buf[w++] = ':';
        for (char c : path) {
            buf[w++] = c;
        }

        len = static_cast<std::uint8_t>(w);

        hash = res_detail::hash_of(ns, path);
    }

    /// @brief Parses "ns:path"; ns defaults to "serenkai" when no colon
    /// present.
    static constexpr std::optional<ResourceLocation>
    parse(std::string_view s) noexcept {
        ResourceLocation r{s};
        if (r.len == 0) {
            return std::nullopt;
        }
        return r;
    }

    constexpr bool valid() const noexcept { return len != 0; }

    constexpr std::string_view str() const noexcept { return {buf, len}; }

    constexpr std::string_view ns() const noexcept { return {buf, ns_len}; }

    constexpr std::string_view path() const noexcept {
        return len > ns_len + 1
                   ? std::string_view{buf + ns_len + 1,
                                      static_cast<std::size_t>(len) - ns_len -
                                          1}
                   : std::string_view{};
    }

    std::string to_string() const { return std::string{str()}; }

    /// @brief Compare for equality byte by byte.
    constexpr bool operator==(const ResourceLocation& o) const noexcept {
        if (len != o.len) {
            return false;
        }

        return str() == o.str();
    }

    constexpr bool operator!=(const ResourceLocation& o) const noexcept {
        return !(*this == o);
    }

private:
    static constexpr bool validate(std::string_view s, std::string_view& out_ns,
                                   std::string_view& out_path) noexcept {
        if (s.empty()) {
            return false;
        }
        if (s.find("..") != std::string_view::npos) {
            return false;
        }
        if (s.front() == '/' || s.front() == ':' || s.back() == ':') {
            return false;
        }

        if (auto it = s.find(':'); it != std::string_view::npos) {
            if (s.find(':', it + 1) != std::string_view::npos) {
                return false; // Multiple ':' illegal.
            }
            out_ns = s.substr(0, it);
            out_path = s.substr(it + 1);
            if (out_ns.empty() || out_path.empty()) {
                return false;
            }
        } else {
            out_ns = DEFAULT_NAMESPACE;
            out_path = s;
        }

        if (out_path.front() == '/') {
            return false;
        }

        const std::size_t total = out_ns.size() + 1 + out_path.size();
        if (total > CAPACITY) {
            return false;
        }

        for (char c : out_ns) {
            if (!res_detail::is_valid_ns(c)) {
                return false;
            }
        }

        for (char c : out_path) {
            if (!res_detail::is_valid_path(c)) {
                return false;
            }
        }

        return true;
    }
};

consteval ResourceLocation operator""_rl(const char* s, std::size_t n) {
    ResourceLocation r{std::string_view{s, n}};
    if (!r.valid())
        throw "Invalid ResourceLocation literal";
    return r;
}

} // namespace serenkai

namespace std {
template <> struct hash<serenkai::ResourceLocation> {
    constexpr std::size_t
    operator()(const serenkai::ResourceLocation& r) const noexcept {
        return r.hash;
    }
};
} // namespace std