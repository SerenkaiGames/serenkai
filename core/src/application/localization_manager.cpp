#include "serenkai/application/localization_manager.hpp"

#include "serenkai/resource/asset_manager.hpp"
#include "serenkai/resource/resource_location.hpp"

#include <magic_enum/magic_enum.hpp>
#include <spdlog/spdlog.h>
#include <string_view>

namespace serenkai {
LocalizationManager::LocalizationManager(AssetManager* asset_manager)
    : m_loder(asset_manager) {}

bool LocalizationManager::load_language(Language lang,
                                        const ResourceLocation& po_loc) {
    auto data = m_loder.load(po_loc);
    if (!data) {
        return false;
    }

    add(lang, *data);
    return true;
}

bool LocalizationManager::load_language(Language lang,
                                        std::string_view po_loc) {
    auto data = m_loder.load(po_loc);
    if (!data) {
        return false;
    }

    add(lang, *data);
    return true;
}

void LocalizationManager::add(Language lang, const PoData& data) {
    if (data.lang != lang) {
        spdlog::error("Target language {} is unequal with loaded language {}",
                      magic_enum::enum_name(lang),
                      magic_enum::enum_name(data.lang));
        return;
    }

    for (const auto& [key, value] : data.entries) {
        auto [_, inserted] = m_tables[lang].try_emplace(key, value);

        if (!inserted) {
            spdlog::debug("Duplicate key {}", key.str());
        }
    }
}

void LocalizationManager::set_language(Language lang) { m_current_lang = lang; }

Language LocalizationManager::current_language() const {
    return m_current_lang;
}

std::string_view
LocalizationManager::lookup(const ResourceLocation& key) const {
    auto table = m_tables.find(m_current_lang);
    if (table == m_tables.end()) {
        spdlog::debug("Language {} not load",
                      magic_enum::enum_name(m_current_lang));
        return lookup_fallback(key);
    }

    auto s = table->second.find(key);
    if (s == table->second.end()) {
        return lookup_fallback(key);
    }

    return s->second;
}

std::string_view LocalizationManager::lookup(std::string_view key_str) const {
    auto key = ResourceLocation::parse(key_str);
    if (!key) {
        spdlog::error("Invalid key {}", key_str);
        return key_str;
    }

    return lookup(*key);
}

std::string_view
LocalizationManager::lookup_fallback(const ResourceLocation& key) const {
    auto table = m_tables.find(m_current_lang);
    if (table == m_tables.end()) {
        spdlog::debug("Language {} not load",
                      magic_enum::enum_name(m_current_lang));
        return key.str();
    }

    auto s = table->second.find(key);
    if (s == table->second.end()) {
        return key.str();
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

    return table->second.contains(key);
}

bool LocalizationManager::has(std::string_view key_str) const {
    auto key = ResourceLocation::parse(key_str);
    if (!key) {
        spdlog::error("Invalid key {}", key_str);
        return false;
    }

    return has(*key);
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