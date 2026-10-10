#include "serenkai/script/script_engine.hpp"

#include "serenkai/resource/asset_manager.hpp"
#include "serenkai/resource/resource_location.hpp"
#include "serenkai/script/bindings/bind_log.hpp"
#include "serenkai/script/lua_compile_options.hpp"
#include "serenkai/script/script.hpp"

#include <exception>
#include <fstream>
#include <ios>
#include <lua.h>
#include <luacode.h>
#include <lualib.h>
#include <optional>
#include <spdlog/spdlog.h>
#include <stdexcept>
#include <string>
#include <utility>
namespace {
void init_lua(lua_State*& L) {
    L = luaL_newstate();
    if (!L) {
        throw std::runtime_error("Failed to create lua state");
    }

    luaL_openlibs(L);
}

void cleanup_lua(lua_State*& L) { lua_close(L); }

} // namespace

namespace serenkai {
ScriptEngine::ScriptEngine(AssetManager* asset_manager)
    : m_asset_manager(asset_manager), m_root(init_lua, cleanup_lua) {

    register_lua_log(m_root.get());

    luabridge::getGlobalNamespace(m_root.get())
        .addFunction("require", [this](const std::string& module_name) {
            return require(module_name);
        });

    // Enable sandbox on root state after all built-in bindings are registered.
    // After all global bindings are registered, enable the read-only sandbox on
    // the root environment.
    luaL_sandbox(m_root.get());
}

bool ScriptEngine::run_string(lua_State* L, const std::string& script,
                              const std::string& chunk_name) {
    size_t bytecode_size = 0;

    auto options = detail::get_lua_options();

    char* bytecode =
        luau_compile(script.data(), script.size(), &options, &bytecode_size);
    if (!bytecode) {
        spdlog::error("luau_compile failed for chunk: {}", chunk_name);
        return false;
    }

    int load_result =
        luau_load(L, chunk_name.c_str(), bytecode, bytecode_size, 0);
    free(bytecode);

    if (load_result != 0) {
        const char* err = lua_tostring(L, -1);
        spdlog::error("luau_load failed: {}", err ? err : "Unknown error");
        lua_pop(L, 1);
        return false;
    }

    if (lua_resume(L, nullptr, 0) != LUA_OK) {
        const char* err = lua_tostring(L, -1);
        spdlog::error("Lua runtime error: {}", err ? err : "Unknown error");
        lua_pop(L, 1);
        return false;
    }

    return true;
}

bool ScriptEngine::run_file(lua_State* L, const std::string& path) {

    std::ifstream file(path, std::ios::binary);
    if (!file) {
        spdlog::error("Cannot open script file: {}", path);
        return false;
    }

    std::ostringstream oss;
    oss << file.rdbuf();
    return run_string(L, oss.str(), path);
}

std::optional<Script> ScriptEngine::create(std::string_view loc) {

    auto res = ResourceLocation::parse(loc);
    if (!res) {
        spdlog::error("Invalid loc {}", loc);
        return std::nullopt;
    }
    return create(*res);
}

std::optional<Script> ScriptEngine::create(const ResourceLocation& loc) {
    if (!m_asset_manager) {
        spdlog::error("Failed to load script {}, assets manager is nullptr",
                      loc.str());
        return std::nullopt;
    }

    auto path = m_asset_manager->get(loc);

    if (!path) {
        spdlog::error("Failed to find {} path", loc.str());
        return std::nullopt;
    }

    try {
        std::ifstream file(*path, std::ios::binary);
        if (!file) {
            spdlog::error("Cannot open script file: {}", *path);
            return std::nullopt;
        }

        std::ostringstream oss;
        oss << file.rdbuf();

        auto state = lua_newthread(m_root.get());
        luaL_sandboxthread(state);

        auto id = lua_ref(m_root.get(), -1);
        lua_pop(m_root.get(), 1);

        LuaState l{[state](lua_State*& L) { L = state; },
                   [id, root = m_root.get()](lua_State*&) {
                       if (id != LUA_NOREF && id != LUA_REFNIL) {
                           lua_unref(root, id);
                       }
                   }};
        return Script{std::move(l), oss.str(), loc.str()};

    } catch (const std::exception& e) {
        spdlog::error("Failed to load {} script, {}", loc.str(), e.what());
        return std::nullopt;
    }
}

luabridge::LuaRef ScriptEngine::require(const std::string& module_name) {

    auto nil = luabridge::LuaRef(m_root.get());

    auto res = ResourceLocation::parse(module_name);
    if (!res) {
        spdlog::error("Invalid module name {}", module_name);
        return nil;
    }

    auto it = m_module_cache.find(*res);
    if (it != m_module_cache.end()) {
        return it->second;
    }

    if (!m_asset_manager) {
        spdlog::error("Failed to require module {}: Asset manager is null",
                      module_name);
        return nil;
    }

    auto path = m_asset_manager->get(module_name);
    if (!path) {
        spdlog::error("Failed to find module {}", module_name);

        return nil;
    }

    // Load and execute in a separate temporary sandbox thread to avoid
    // reentrancy conflicts with the caller's coroutine.
    auto module_thread = lua_newthread(m_root.get());
    luaL_sandboxthread(module_thread);

    if (!run_file(module_thread, *path)) {
        spdlog::error("Failed to load module: {}", module_name);
        lua_pop(m_root.get(), 1);
        return nil;
    }
    // Move the module return value from module_thread to the top of the
    // m_root_state stack.
    // Attach the result to the root state to avoid dangling pointers.
    luabridge::LuaRef result(m_root.get());
    if (lua_gettop(module_thread) > 0) {
        lua_xmove(module_thread, m_root.get(), 1);
        result = luabridge::LuaRef::fromStack(m_root.get());
    } else {
        result = luabridge::LuaRef(m_root.get(), true);
    }

    lua_pop(m_root.get(), 1);

    m_module_cache.try_emplace(*res, result);

    return result;
}

} // namespace serenkai