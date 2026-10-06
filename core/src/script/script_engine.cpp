#include "serenkai/script/script_engine.hpp"

#include "serenkai/resource/asset_manager.hpp"
#include "serenkai/resource/resource_location.hpp"
#include "serenkai/script/bindings/bind_log.hpp"

#include <fstream>
#include <lua.h>
#include <luacode.h>
#include <lualib.h>
#include <optional>
#include <spdlog/spdlog.h>
#include <stdexcept>
#include <string>
namespace {
void init_lua(lua_State*& L) {
    L = luaL_newstate();
    if (!L) {
        throw std::runtime_error("Failed to create lua state");
    }

    luaL_openlibs(L);
}

void cleanup_lua(lua_State*& L) { lua_close(L); }

lua_CompileOptions get_lua_options() {
    lua_CompileOptions options{};

#ifdef NDEBUG
    // Release
    options.optimizationLevel = 2;
    options.debugLevel = 0;
#else
    // Debug
    options.optimizationLevel = 1;
    options.debugLevel = 2;
#endif
    options.typeInfoLevel = 1;

    return options;
}

} // namespace

namespace serenkai {
ScriptEngine::ScriptEngine(AssetManager* asset_manager)
    : m_asset_manager(asset_manager), m_root_state(init_lua, cleanup_lua) {

    register_lua_log(m_root_state.get());

    luabridge::getGlobalNamespace(m_root_state.get())
        .addFunction("require", [this](const std::string& module_name) {
            return require(module_name);
        });

    // Enable sandbox on root state after all built-in bindings are registered.
    // After all global bindings are registered, enable the read-only sandbox on
    // the root environment.
    luaL_sandbox(m_root_state.get());
}

bool ScriptEngine::run_string(lua_State* L, const std::string& script,
                              const std::string& chunk_name) {
    size_t bytecode_size = 0;

    auto options = get_lua_options();

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

bool ScriptEngine::load(std::string_view loc) {

    if (!m_asset_manager) {
        spdlog::error("Failed to load script {}, assets manager is nullptr",
                      loc);
        return false;
    }

    auto res = ResourceLocation::parse(loc);
    if (!res) {
        spdlog::error("Invalid loc {}", loc);
        return false;
    }
    if (m_states.find(*res) != m_states.end()) {
        spdlog::warn("Script {} already loaded, do nothing", loc);
        return false;
    }

    auto state = lua_newthread(m_root_state.get());
    luaL_sandboxthread(state);

    auto id = lua_ref(m_root_state.get(), -1);
    lua_pop(m_root_state.get(), 1);

    auto path = m_asset_manager->get(loc);
    if (path && run_file(state, *path)) {
        m_states.try_emplace(*res, state);
        m_thread_refs.try_emplace(*res, id);
        return true;
    } else {
        spdlog::error("Failed to load script {}", loc);
        lua_unref(m_root_state.get(), id);
        return false;
    }
}

bool ScriptEngine::unload(std::string_view loc) {
    auto res = ResourceLocation::parse(loc);
    if (!res) {
        spdlog::error("Invalid loc {}", loc);
        return false;
    }
    auto l_it = m_states.find(*res);
    auto id_it = m_thread_refs.find(*res);
    if (l_it == m_states.end() || id_it == m_thread_refs.end()) {
        spdlog::warn("Can't find script {} in states map", loc);
        return false;
    }
    auto id = id_it->second;
    if (id != LUA_NOREF && id != LUA_REFNIL) {
        lua_unref(m_root_state.get(), id);
        id = LUA_NOREF;
    }

    m_states.erase(l_it);
    m_thread_refs.erase(id_it);
    return true;
}

std::optional<luabridge::LuaRef>
ScriptEngine::get_global(std::string_view loc, std::string_view global) {
    auto res = ResourceLocation::parse(loc);

    if (!res) {
        spdlog::error("Invalid loc {}", loc);
        return std::nullopt;
    }

    return get_global(loc, global);
}

std::optional<luabridge::LuaRef>
ScriptEngine::get_global(const ResourceLocation& loc, std::string_view global) {
    auto it = m_states.find(loc);
    if (it == m_states.end()) {
        return std::nullopt;
    }
    auto var = luabridge::getGlobal(it->second, std::string(global).c_str());
    if (var.isNil()) {
        return std::nullopt;
    }
    return var;
}

luabridge::LuaRef ScriptEngine::require(const std::string& module_name) {

    auto nil = luabridge::LuaRef(m_root_state.get());

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
    auto module_thread = lua_newthread(m_root_state.get());
    luaL_sandboxthread(module_thread);

    if (!run_file(module_thread, *path)) {
        spdlog::error("Failed to load module: {}", module_name);
        lua_pop(m_root_state.get(), 1);
        return nil;
    }
    // Move the module return value from module_thread to the top of the
    // m_root_state stack.
    // Attach the result to the root state to avoid dangling pointers.
    luabridge::LuaRef result(m_root_state.get());
    if (lua_gettop(module_thread) > 0) {
        lua_xmove(module_thread, m_root_state.get(), 1);
        result = luabridge::LuaRef::fromStack(m_root_state.get());
    } else {
        result = luabridge::LuaRef(m_root_state.get(), true);
    }

    lua_pop(m_root_state.get(), 1);

    m_module_cache.try_emplace(*res, result);

    return result;
}

} // namespace serenkai