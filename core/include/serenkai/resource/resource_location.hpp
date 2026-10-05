#pragma once

#include "serenkai/base/math.hpp"

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>

namespace serenkai {

/// @brief Defines a struct for the name and path of a resource file
///
/// Uses a namespace-based approach, consisting of namespace and path; path uses
/// '/' For example, a resource path can be example:texture/example.png
/// @note If no namespace is specified, the default is serenkai
/// @note Only uppercase letters, lowercase letters, digits, and the characters
/// '.', '_', ':', '/', '-' are allowed.
struct ResourceLocation {
    static constexpr std::string_view DEFAULT_NAMESPACE = "serenkai";
    const std::string ns;
    const std::string path;
    const size_t hash;

    ResourceLocation(std::string_view ns, std::string_view path)
        : ns(ns), path(path),
          hash(combine32(std::hash<std::string_view>{}(ns),
                         std::hash<std::string_view>{}(path))) {}

    std::string to_string() const { return ns + ":" + path; }

    static bool is_valid_path(char c) {
        return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
               (c >= '0' && c <= '9') || c == '.' || c == '_' || c == ':' ||
               c == '/' || c == '-';
    }

    static bool is_valid_ns(char c) {
        return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
               (c >= '0' && c <= '9') || c == '_' || c == '-';
    }

    /// @brief Parses "ns:path"; ns defaults to "serenkai" when no colon
    /// present.
    static std::optional<ResourceLocation> parse(std::string_view str) {
        if (str.empty() || str.contains("..") || str.front() == '/' ||
            str.front() == ':' || str.back() == ':') {
            return std::nullopt;
        }

        auto it = str.find(':');
        std::string_view ns = DEFAULT_NAMESPACE;
        std::string_view path = str;
        if (it != std::string_view::npos) {
            if (str.find(':', it + 1) != std::string_view::npos) {
                return std::nullopt; // only one colon allowed
            }
            ns = str.substr(0, it);
            path = str.substr(it + 1);
            if (ns.empty() || path.empty()) {
                return std::nullopt;
            }
        }

        if (path.front() == '/') {
            return std::nullopt;
        }

        for (char c : ns) {
            if (!is_valid_ns(c)) {
                return std::nullopt;
            }
        }

        for (char c : path) {
            if (!is_valid_path(c))
                return std::nullopt;
        }

        return ResourceLocation{ns, path};
    }

    bool operator==(const ResourceLocation& o) const {
        return (ns == o.ns) && (path == o.path);
    }
};
} // namespace serenkai

namespace std {
template <> struct hash<serenkai::ResourceLocation> {
    std::size_t operator()(const serenkai::ResourceLocation& p) const noexcept {
        return p.hash;
    }
};
} // namespace std