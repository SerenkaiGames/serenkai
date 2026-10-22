#include "serenkai/script/bindings/bind_localization.hpp"

#include "serenkai/application/localization_manager.hpp"
#include "serenkai/script/lua_header.hpp"

#include <spdlog/spdlog.h>
#include <string>

namespace serenkai {

void bind_localization_to_lua(lua_State* L, LocalizationManager* mgr) {
    if (!mgr) {
        spdlog::debug(
            "Failed to bind localization,LocalizationManager is nullptr");
        return;
    }

    luabridge::getGlobalNamespace(L).addFunction(
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
        });
}
} // namespace serenkai