#include "serenkai/script/bindings/bind_ui.hpp"

#include "serenkai/gui/ui_manager.hpp"
#include "serenkai/script/bindings/facade_wrapper.hpp"
#include "serenkai/script/lua_header.hpp"
#include "serenkai/script/script_engine.hpp"

#include <spdlog/spdlog.h>
#include <string_view>

#define BIND(name, ...)                                                        \
    addFunction(name, wrap_facade(get_subsystem, __VA_ARGS__))

namespace serenkai {
void bind_ui_to_lua(lua_State* L, ScriptEngine* engine) {

    auto get_subsystem = [engine]() -> UiManager* {
        if (!engine) {
            return nullptr;
        }
        return engine->ui_mgr_context().ui_mgr;
    };

    luabridge::getGlobalNamespace(L)
        .beginNamespace("Ui")
        .BIND("open", [](UiManager* mgr,
                         std::string_view loc) { mgr->request_open(loc); })
        .BIND("close", [](UiManager* mgr,
                          std::string_view loc) { mgr->request_close(loc); })
        .BIND("clear", [](UiManager* mgr) { mgr->request_clear(); })
        .BIND("reload", [](UiManager* mgr) { mgr->request_reload_all(); })
        .endNamespace();
}
} // namespace serenkai