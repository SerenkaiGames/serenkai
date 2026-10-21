#pragma once

#include "serenkai/base/language.hpp"
#include "serenkai/resource/po_data.hpp"
#include "serenkai/resource/po_loader.hpp"
#include "serenkai/resource/resource_location.hpp"

#include <fmt/format.h>
#include <functional>
#include <string_view>
#include <unordered_map>
namespace serenkai {

class AssetManager;

/// @brief Manages game localization, language switching, and text translation.
///
/// Loads PO translation files via PoLoader and provides fast lookup by
/// ResourceLocation.
class LocalizationManager {
public:
    using LanguageCallback = std::function<void(Language)>;

    struct FormatArg {
        std::string name;
        std::string value;
    };

    explicit LocalizationManager(AssetManager* asset_manager);

    bool load_language(Language lang, const ResourceLocation& po_loc);

    bool load_language(Language lang, std::string_view po_loc);

    /// @brief Set the currently used language; does not automatically load
    /// language files.
    void set_language(Language lang);

    Language current_language() const;

    /// @brief Check whether the key exists; if it is missing in the current
    /// language, check the fallback language.
    bool has(const ResourceLocation& key) const;

    bool has(std::string_view key) const;

    template <typename T> FormatArg arg(std::string_view name, T&& value) {
        return {std::string(name), fmt::format("{}", std::forward<T>(value))};
    }

    template <typename... Args>
    std::string translate(std::string_view key, Args&&... args) {
        if constexpr (sizeof...(Args) == 0) {
            return std::string{lookup(key)};
        }
        std::string fmt{lookup(key)};
        (replace_all(fmt, "{" + args.name + "}", args.value), ...);
        return fmt;
    }

private:
    using TranslationTable = std::unordered_map<ResourceLocation, std::string>;

    PoLoader m_loder;

    Language m_current_lang = Language::en_US;
    Language m_fallback_lang = Language::en_US;

    // Cache translations per language: Language -> (Key -> Translated Text)
    std::unordered_map<Language, TranslationTable> m_tables;

    void add(Language lang, const PoData& data);

    /// @brief Look up the corresponding text by key; does not process {}.
    /// @warning Ensure that the key is not a temporary variable.
    std::string_view lookup(const ResourceLocation& key) const;

    std::string_view lookup(std::string_view key_str) const;

    /// @warning Ensure that the key is not a temporary variable.
    std::string_view lookup_fallback(const ResourceLocation& key) const;

    void replace_all(std::string& str, std::string_view from,
                     std::string_view to);
};
} // namespace serenkai