#include "serenkai/application/localization_manager.hpp"

#include "serenkai/resource/asset_manager.hpp"
#include "serenkai/resource/resource_location.hpp"

#include <magic_enum/magic_enum.hpp>
#include <optional>
#include <spdlog/spdlog.h>
#include <string>
#include <string_view>

namespace serenkai {
LocalizationManager::LocalizationManager(AssetManager* asset_manager)
    : m_loader(asset_manager) {}

bool LocalizationManager::load_language(Language lang,
                                        const ResourceLocation& po_loc) {
    auto data = m_loader.load(po_loc);
    if (!data) {
        return false;
    }

    return add(lang, *data);
}

bool LocalizationManager::load_language(Language lang,
                                        std::string_view po_loc) {
    auto data = m_loader.load(po_loc);
    if (!data) {
        return false;
    }

    return add(lang, *data);
}

bool LocalizationManager::add(Language lang, const PoData& data) {
    if (data.lang != lang) {
        spdlog::error("Target language {} does not match loaded language {}",
                      magic_enum::enum_name(lang),
                      magic_enum::enum_name(data.lang));
        return false;
    }

    for (const auto& [key, value] : data.entries) {
        auto [_, inserted] = m_tables[lang].try_emplace(key, value);

        if (!inserted) {
            spdlog::debug("Duplicate key {}", key.str());
        }
    }
    return true;
}

void LocalizationManager::set_language(Language lang) { m_current_lang = lang; }

Language LocalizationManager::current_language() const {
    return m_current_lang;
}

void LocalizationManager::set_fallback_language(Language lang) {
    m_fallback_lang = lang;
}

Language LocalizationManager::fallback_language() const {
    return m_fallback_lang;
}

std::optional<std::string_view>
LocalizationManager::lookup(const ResourceLocation& key) const {
    auto table = m_tables.find(m_current_lang);
    if (table == m_tables.end()) {
        spdlog::debug("Language {} is not loaded",
                      magic_enum::enum_name(m_current_lang));
        return lookup_fallback(key);
    }

    auto s = table->second.find(key);
    if (s == table->second.end()) {
        return lookup_fallback(key);
    }

    return s->second;
}

std::optional<std::string_view>
LocalizationManager::lookup(std::string_view key_str) const {
    auto key = ResourceLocation::parse(key_str);
    if (!key) {
        spdlog::error("Invalid key {}", key_str);
        return std::nullopt;
    }

    return lookup(*key);
}

std::optional<std::string_view>
LocalizationManager::lookup_fallback(const ResourceLocation& key) const {
    auto table = m_tables.find(m_fallback_lang);
    if (table == m_tables.end()) {
        spdlog::debug("Language {} not load",
                      magic_enum::enum_name(m_fallback_lang));
        return std::nullopt;
    }

    auto s = table->second.find(key);
    if (s == table->second.end()) {
        return std::nullopt;
    }

    return s->second;
}

bool LocalizationManager::has(const ResourceLocation& key) const {
    auto table = m_tables.find(m_current_lang);
    if (table == m_tables.end()) {
        spdlog::debug("Language {} not load",
                      magic_enum::enum_name(m_current_lang));
        auto fallback = m_tables.find(m_fallback_lang);
        if (fallback == m_tables.end()) {
            spdlog::debug("Language {} not load",
                          magic_enum::enum_name(m_fallback_lang));
            return false;
        }

        return fallback->second.contains(key);
    }

    if (table->second.contains(key)) {
        return true;
    }
    auto fallback = m_tables.find(m_fallback_lang);
    if (fallback == m_tables.end()) {
        spdlog::debug("Language {} not load",
                      magic_enum::enum_name(m_fallback_lang));
        return false;
    }

    return fallback->second.contains(key);
}

bool LocalizationManager::has(std::string_view key_str) const {
    auto key = ResourceLocation::parse(key_str);
    if (!key) {
        spdlog::error("Invalid key {}", key_str);
        return false;
    }

    return has(*key);
}

std::string LocalizationManager::translate(std::string_view key,
                                           std::span<const FormatArg> args) {
    auto s = lookup(key);
    if (!s) {
        return std::string(key);
    }
    std::string fmt{*s};
    for (auto& arg : args) {
        replace_all(fmt, "{" + arg.name + "}", arg.value);
    }
    return fmt;
}

std::string LocalizationManager::translate(const ResourceLocation& key,
                                           std::span<const FormatArg> args) {
    auto s = lookup(key);
    if (!s) {
        return key.to_string();
    }
    std::string fmt{*s};
    for (auto& arg : args) {
        replace_all(fmt, "{" + arg.name + "}", arg.value);
    }
    return fmt;
}

void LocalizationManager::replace_all(std::string& str, std::string_view from,
                                      std::string_view to) {
    std::size_t pos = 0;

    while ((pos = str.find(from, pos)) != std::string::npos) {
        str.replace(pos, from.size(), to);
        pos += to.size();
    }
}

} // namespace serenkai