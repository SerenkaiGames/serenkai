#include "serenkai/script/bindings/bind_localization.hpp"

#include "serenkai/application/localization_manager.hpp"
#include "serenkai/base/language.hpp"
#include "serenkai/script/lua_header.hpp"

#include <magic_enum/magic_enum.hpp>
#include <spdlog/spdlog.h>
#include <string>
#include <string_view>

namespace serenkai {

void bind_localization_to_lua(lua_State* L, LocalizationManager* mgr) {
    if (!mgr) {
        spdlog::debug(
            "Failed to bind localization, localization manager is nullptr");
        return;
    }

    luabridge::getGlobalNamespace(L)
        .addFunction(
            "Tr",
            [mgr](std::string_view key) -> std::string {
                return mgr->translate(key);
            },
            [mgr](std::string_view key,
                  const std::unordered_map<std::string, std::string>& table)
                -> std::string {
                std::vector<LocalizationManager::FormatArg> args;
                args.reserve(table.size());
                for (const auto& [k, v] : table) {
                    args.push_back({k, v});
                }
                return mgr->translate(key, args);
            })
        .beginNamespace("Loc")
        .addFunction(
            "current",
            [mgr]() { return magic_enum::enum_name(mgr->current_language()); })
        .addFunction(
            "fallback",
            [mgr]() { return magic_enum::enum_name(mgr->fallback_language()); })
        .addFunction("set",
                     [mgr](std::string_view lang_str) -> bool {
                         auto lang = magic_enum::enum_cast<Language>(lang_str);
                         if (!lang) {
                             spdlog::error("Invalid language {}", lang_str);
                             return false;
                         }
                         mgr->set_language(*lang);
                         return true;
                     })
        .addFunction("set_fallback",
                     [mgr](std::string_view lang_str) -> bool {
                         auto lang = magic_enum::enum_cast<Language>(lang_str);
                         if (!lang) {
                             spdlog::error("Invalid language {}", lang_str);
                             return false;
                         }
                         mgr->set_fallback_language(*lang);
                         return true;
                     })
        .addFunction("load",
                     [mgr](std::string_view lang_str, std::string_view loc) {
                         auto lang = magic_enum::enum_cast<Language>(lang_str);
                         if (!lang) {
                             spdlog::error("Invalid language {}", lang_str);
                             return false;
                         }
                         return mgr->load_language(*lang, loc);
                     })
        .addFunction("has",
                     [mgr](std::string_view key) { return mgr->has(key); })
        .endNamespace();
}
} // namespace serenkai