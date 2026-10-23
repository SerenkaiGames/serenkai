#include "serenkai/application/localization_manager.hpp"
#include "serenkai/base/language.hpp"
#include "serenkai/game/camera.hpp"
#include "serenkai/game/entity_manager.hpp"
#include "serenkai/game/map.hpp"
#include "serenkai/game/map_data.hpp"
#include "serenkai/game/map_manager.hpp"
#include "serenkai/resource/asset_manager.hpp"
#include "serenkai/resource/asset_source.hpp"
#include "serenkai/resource/resource_location.hpp"
#include "serenkai/script/script.hpp"
#include "serenkai/script/script_engine.hpp"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <string_view>
#include <utility>

namespace fs = std::filesystem;
using namespace serenkai;

namespace {

/// @brief Mock AssetSource for in-memory asset mapping.
class MockScriptSource : public AssetSource {
public:
    explicit MockScriptSource(AssetFileMap files) : m_files(std::move(files)) {}

    AssetFileMap& get_asset_files() override { return m_files; }

    std::string source_name() const override { return "MockScriptSource"; }

private:
    AssetFileMap m_files;
};

/// @brief Helper environment for creating and registering script files.
class TestScriptEnv {
public:
    TestScriptEnv() {
        auto timestamp =
            std::chrono::steady_clock::now().time_since_epoch().count();
        m_temp_dir = fs::temp_directory_path() /
                     ("serenkai_test_script_" + std::to_string(timestamp));
        fs::create_directories(m_temp_dir);
    }

    ~TestScriptEnv() {
        std::error_code ec;
        fs::remove_all(m_temp_dir, ec);
    }

    void register_file(std::string_view loc_str, const std::string& content,
                       std::string_view ext = ".luau") {
        auto loc = ResourceLocation::parse(loc_str);
        if (!loc) {
            return;
        }

        auto file_name = std::string(loc->ns()) + "_" +
                         std::to_string(m_counter++) + std::string(ext);
        auto file_path = m_temp_dir / file_name;
        {
            std::ofstream out(file_path);
            out << content;
        }

        AssetFileMap files;
        files.emplace(*loc, file_path.string());
        m_asset_manager.merge_source(
            std::make_shared<MockScriptSource>(std::move(files)));
    }

    void register_script(std::string_view loc_str, const std::string& content) {
        register_file(loc_str, content, ".luau");
    }

    AssetManager& asset_manager() { return m_asset_manager; }

private:
    fs::path m_temp_dir;
    AssetManager m_asset_manager;
    size_t m_counter = 0;
};

} // namespace

TEST_CASE("ScriptEngine script creation and lifecycle", "[script]") {
    TestScriptEnv env;
    env.register_script("test:valid.luau",
                        "function test_func() return 42 end");
    env.register_script("test:syntax_err.luau",
                        "this is invalid lua code !@#$%");
    env.register_script("test:runtime_err.luau",
                        "error('runtime error on load')");

    ScriptEngine engine(
        ScriptEngineConfig{.asset_manager = &env.asset_manager()});

    SECTION("Create valid script succeeds") {
        auto script = engine.create("test:valid.luau");
        CHECK(script.has_value());
    }

    SECTION("Multiple create calls return independent scripts") {
        auto s1 = engine.create("test:valid.luau");
        auto s2 = engine.create("test:valid.luau");
        CHECK(s1.has_value());
        CHECK(s2.has_value());
    }

    SECTION("Create with invalid resource location returns nullopt") {
        CHECK_FALSE(engine.create("").has_value());
        CHECK_FALSE(engine.create("invalid::loc").has_value());
    }

    SECTION("Create non-existent asset returns nullopt") {
        CHECK_FALSE(engine.create("test:non_existent.luau").has_value());
    }

    SECTION("Create script with syntax error returns nullopt") {
        CHECK_FALSE(engine.create("test:syntax_err.luau").has_value());
    }

    SECTION("Create script with runtime error on execution returns nullopt") {
        CHECK_FALSE(engine.create("test:runtime_err.luau").has_value());
    }

    SECTION("Create with null AssetManager returns nullopt") {
        ScriptEngine null_engine(ScriptEngineConfig{.asset_manager = nullptr});
        CHECK_FALSE(null_engine.create("test:valid.luau").has_value());
    }

    SECTION("Script destruction frees resources and recreating succeeds") {
        {
            auto script = engine.create("test:valid.luau");
            CHECK(script.has_value());
        }
        auto script2 = engine.create("test:valid.luau");
        CHECK(script2.has_value());
    }
}

TEST_CASE("Script function call invocation", "[script]") {
    TestScriptEnv env;
    env.register_script("test:math.luau", R"(
        function add(a: number, b: number): number
            return a + b
        end

        function greet(name: string): string
            return "hello " .. name
        end

        function get_pair(): (number, string)
            return 100, "player"
        end

        function no_return()
            -- no return value
        end

        function failing_func()
            error("deliberate failure")
        end

        not_a_func = 12345
    )");

    ScriptEngine engine(
        ScriptEngineConfig{.asset_manager = &env.asset_manager()});
    auto script = engine.create("test:math.luau");
    REQUIRE(script.has_value());

    SECTION("Call function with arguments and single return value") {
        auto res = script->call<int>("add", 15, 27);
        REQUIRE(res.has_value());
        CHECK(std::get<0>(*res) == 42);
    }

    SECTION("Create script using ResourceLocation overload and _rl literal") {
        auto res_script = engine.create("test:math.luau"_rl);
        REQUIRE(res_script.has_value());
        auto res = res_script->call<int>("add", 15, 27);
        REQUIRE(res.has_value());
        CHECK(std::get<0>(*res) == 42);
    }

    SECTION("Call function with string argument and string return value") {
        auto res = script->call<std::string>("greet", "serenkai");
        REQUIRE(res.has_value());
        CHECK(std::get<0>(*res) == "hello serenkai");
    }

    SECTION("Call function with multiple return values") {
        auto res = script->call<int, std::string>("get_pair");
        REQUIRE(res.has_value());
        CHECK(std::get<0>(*res) == 100);
        CHECK(std::get<1>(*res) == "player");
    }

    SECTION("Call function with no return values") {
        auto res = script->call<>("no_return");
        CHECK(res.has_value());
    }

    SECTION("Call non-existent function returns nullopt") {
        auto res = script->call<int>("non_existent");
        CHECK_FALSE(res.has_value());
    }

    SECTION("Call non-callable global returns nullopt") {
        auto res = script->call<int>("not_a_func");
        CHECK_FALSE(res.has_value());
    }

    SECTION("Call function that errors returns nullopt") {
        auto res = script->call<>("failing_func");
        CHECK_FALSE(res.has_value());
    }
}

TEST_CASE("Script per-resource thread isolation", "[script]") {
    TestScriptEnv env;
    env.register_script("test:script_a.luau", R"(
        shared_val = "value_from_a"
        function get_val()
            return shared_val
        end
    )");
    env.register_script("test:script_b.luau", R"(
        shared_val = "value_from_b"
        function get_val()
            return shared_val
        end
    )");

    ScriptEngine engine(
        ScriptEngineConfig{.asset_manager = &env.asset_manager()});
    auto script_a = engine.create("test:script_a.luau");
    auto script_b = engine.create("test:script_b.luau");
    REQUIRE(script_a.has_value());
    REQUIRE(script_b.has_value());

    SECTION("Globals in different script threads are isolated") {
        auto res_a = script_a->call<std::string>("get_val");
        auto res_b = script_b->call<std::string>("get_val");

        REQUIRE(res_a.has_value());
        REQUIRE(res_b.has_value());
        CHECK(std::get<0>(*res_a) == "value_from_a");
        CHECK(std::get<0>(*res_b) == "value_from_b");
    }

    SECTION("Destructing one script does not affect the other") {
        script_a.reset();

        // script_b is still accessible
        auto res_b = script_b->call<std::string>("get_val");
        REQUIRE(res_b.has_value());
        CHECK(std::get<0>(*res_b) == "value_from_b");
    }
}

TEST_CASE("Script logging bindings and standard libraries", "[script]") {
    TestScriptEnv env;
    env.register_script("test:logging.luau", R"(
        function test_logging()
            print("Print message from script")
            print("Multiple", "args", 123, true)
            log.info("Info log message")
            log.warn("Warn log message")
            log.error("Error log message")
            log.debug("Debug log message")
            return true
        end

        function test_stdlib()
            local s = string.upper("hello")
            local m = math.max(10, 20)
            return s, m
        end
    )");

    ScriptEngine engine(
        ScriptEngineConfig{.asset_manager = &env.asset_manager()});
    auto script = engine.create("test:logging.luau");
    REQUIRE(script.has_value());

    SECTION("Logging functions execute without error") {
        auto res = script->call<bool>("test_logging");
        REQUIRE(res.has_value());
        CHECK(std::get<0>(*res) == true);
    }

    SECTION("Standard library functions work in sandboxed thread") {
        auto res = script->call<std::string, int>("test_stdlib");
        REQUIRE(res.has_value());
        CHECK(std::get<0>(*res) == "HELLO");
        CHECK(std::get<1>(*res) == 20);
    }
}

TEST_CASE("Script module require and caching", "[script]") {
    TestScriptEnv env;
    env.register_script("test:math_lib.luau", R"(
        local M = {}
        function M.add(a: number, b: number): number
            return a + b
        end
        M.version = "1.0.0"
        return M
    )");

    env.register_script("test:counter.luau", R"(
        local M = { count = 0 }
        function M.increment(): number
            M.count = M.count + 1
            return M.count
        end
        return M
    )");

    env.register_script("test:consumer_a.luau", R"(
        local math_lib = require("test:math_lib.luau")
        local counter = require("test:counter.luau")

        function calc(a: number, b: number): number
            return math_lib.add(a, b)
        end

        function get_version(): string
            return math_lib.version
        end

        function inc(): number
            return counter.increment()
        end
    )");

    env.register_script("test:consumer_b.luau", R"(
        local counter = require("test:counter.luau")

        function inc(): number
            return counter.increment()
        end
    )");

    env.register_script("test:invalid_require.luau", R"(
        local bad = require("test:non_existent.luau")
        function is_bad_nil(): boolean
            return bad == nil
        end
    )");

    ScriptEngine engine(
        ScriptEngineConfig{.asset_manager = &env.asset_manager()});

    SECTION("Require loads module and allows calling its functions") {
        auto script = engine.create("test:consumer_a.luau");
        REQUIRE(script.has_value());
        auto res = script->call<int>("calc", 10, 20);
        REQUIRE(res.has_value());
        CHECK(std::get<0>(*res) == 30);

        auto ver = script->call<std::string>("get_version");
        REQUIRE(ver.has_value());
        CHECK(std::get<0>(*ver) == "1.0.0");
    }

    SECTION(
        "Multiple scripts requiring the same module share cached instance") {
        auto script_a = engine.create("test:consumer_a.luau");
        auto script_b = engine.create("test:consumer_b.luau");
        REQUIRE(script_a.has_value());
        REQUIRE(script_b.has_value());

        auto res_a = script_a->call<int>("inc");
        REQUIRE(res_a.has_value());
        CHECK(std::get<0>(*res_a) == 1);

        // consumer_b should access the same cached module table
        auto res_b = script_b->call<int>("inc");
        REQUIRE(res_b.has_value());
        CHECK(std::get<0>(*res_b) == 2);
    }

    SECTION("Require non-existent module returns nil") {
        auto script = engine.create("test:invalid_require.luau");
        REQUIRE(script.has_value());
        auto res = script->call<bool>("is_bad_nil");
        REQUIRE(res.has_value());
        CHECK(std::get<0>(*res) == true);
    }
}

TEST_CASE("Script event system module lifecycle and dispatch", "[script]") {
    TestScriptEnv env;
    env.register_script("serenkai:scripts/event.luau", R"(
        local listeners: { [string]: { (any) -> () } } = {}
        local Event = {}

        function Event.on(event: string, callback: (any) -> ())
            local list = listeners[event]
            if not list then
                list = {}
                listeners[event] = list
            end

            table.insert(list, callback)

            return function()
                local idx = table.find(list, callback)
                if idx then
                    table.remove(list, idx)
                end
            end
        end

        function Event.emit(event: string, ...: any)
            local list = listeners[event]
            if not list then
                return
            end
            local l = table.clone(list)
            for _, callback in l do
                local success, err = pcall(callback, ...)
                if not success then
                    local idx = table.find(list, callback)
                    if idx then
                        table.remove(list, idx)
                    end
                end
            end
        end

        return Event
    )");

    ScriptEngine engine(
        ScriptEngineConfig{.asset_manager = &env.asset_manager()});

    SECTION("Event listener registration and emission") {
        env.register_script("test:event_test.luau", R"(
            local Event = require("serenkai:scripts/event.luau")

            local received_value = 0
            local received_text = ""

            Event.on("test:custom_event", function(val: number, text: string)
                received_value = val
                received_text = text
            end)

            function trigger(val: number, text: string)
                Event.emit("test:custom_event", val, text)
            end

            function get_result(): (number, string)
                return received_value, received_text
            end
        )");

        auto script = engine.create("test:event_test.luau");
        REQUIRE(script.has_value());
        auto res = script->call<>("trigger", 42, "hello");
        REQUIRE(res.has_value());

        auto out = script->call<int, std::string>("get_result");
        REQUIRE(out.has_value());
        CHECK(std::get<0>(*out) == 42);
        CHECK(std::get<1>(*out) == "hello");
    }

    SECTION("Event listener unregistration stops receiving events") {
        env.register_script("test:unregister_test.luau", R"(
            local Event = require("serenkai:scripts/event.luau")

            local count = 0
            local unregister = Event.on("test:ping", function()
                count = count + 1
            end)

            function emit_ping()
                Event.emit("test:ping")
            end

            function unregister_listener()
                unregister()
            end

            function get_count(): number
                return count
            end
        )");

        auto script = engine.create("test:unregister_test.luau");
        REQUIRE(script.has_value());
        CHECK(script->call<>("emit_ping").has_value());
        auto c1 = script->call<int>("get_count");
        REQUIRE(c1.has_value());
        CHECK(std::get<0>(*c1) == 1);

        CHECK(script->call<>("unregister_listener").has_value());

        CHECK(script->call<>("emit_ping").has_value());
        auto c2 = script->call<int>("get_count");
        REQUIRE(c2.has_value());
        CHECK(std::get<0>(*c2) == 1);
    }

    SECTION("Multiple listeners receive emitted event") {
        env.register_script("test:multi_listener.luau", R"(
            local Event = require("serenkai:scripts/event.luau")

            local a_called = false
            local b_called = false

            Event.on("test:multi", function()
                a_called = true
            end)

            Event.on("test:multi", function()
                b_called = true
            end)

            function trigger()
                Event.emit("test:multi")
            end

            function get_status(): (boolean, boolean)
                return a_called, b_called
            end
        )");

        auto script = engine.create("test:multi_listener.luau");
        REQUIRE(script.has_value());
        CHECK(script->call<>("trigger").has_value());
        auto status = script->call<bool, bool>("get_status");
        REQUIRE(status.has_value());
        CHECK(std::get<0>(*status) == true);
        CHECK(std::get<1>(*status) == true);
    }

    SECTION("Failing listener is pruned and does not block others") {
        env.register_script("test:failing_listener.luau", R"(
            local Event = require("serenkai:scripts/event.luau")

            local normal_count = 0
            local failing_invoked = 0

            Event.on("test:error_event", function()
                failing_invoked = failing_invoked + 1
                error("simulated listener error")
            end)

            Event.on("test:error_event", function()
                normal_count = normal_count + 1
            end)

            function trigger()
                Event.emit("test:error_event")
            end

            function get_counts(): (number, number)
                return failing_invoked, normal_count
            end
        )");

        auto script = engine.create("test:failing_listener.luau");
        REQUIRE(script.has_value());

        // First trigger: failing listener throws error and normal listener runs
        CHECK(script->call<>("trigger").has_value());
        auto counts1 = script->call<int, int>("get_counts");
        REQUIRE(counts1.has_value());
        CHECK(std::get<0>(*counts1) == 1);
        CHECK(std::get<1>(*counts1) == 1);

        // Second trigger: failing listener should have been pruned, normal
        // listener still runs
        CHECK(script->call<>("trigger").has_value());
        auto counts2 = script->call<int, int>("get_counts");
        REQUIRE(counts2.has_value());
        CHECK(std::get<0>(*counts2) == 1);
        CHECK(std::get<1>(*counts2) == 2);
    }

    SECTION("Emitting event with no listeners is safe") {
        env.register_script("test:empty_emit.luau", R"(
            local Event = require("serenkai:scripts/event.luau")

            function trigger_empty(): boolean
                Event.emit("test:unregistered_event", 1, 2, 3)
                return true
            end
        )");

        auto script = engine.create("test:empty_emit.luau");
        REQUIRE(script.has_value());
        auto res = script->call<bool>("trigger_empty");
        REQUIRE(res.has_value());
        CHECK(std::get<0>(*res) == true);
    }

    SECTION("Script on_update dispatches serenkai:events/update") {
        env.register_script("test:main.luau", R"(
            local Event = require("serenkai:scripts/event.luau")

            function on_update(dt: number)
                Event.emit("serenkai:events/update", dt)
            end
        )");

        env.register_script("test:update_listener.luau", R"(
            local Event = require("serenkai:scripts/event.luau")

            local last_dt = 0.0

            Event.on("serenkai:events/update", function(dt: number)
                last_dt = dt
            end)

            function get_dt(): number
                return last_dt
            end
        )");

        auto listener = engine.create("test:update_listener.luau");
        auto main = engine.create("test:main.luau");
        REQUIRE(listener.has_value());
        REQUIRE(main.has_value());

        // Trigger on_update through main script
        auto call_res = main->call<>("on_update", 0.016f);
        REQUIRE(call_res.has_value());

        auto dt_res = listener->call<double>("get_dt");
        REQUIRE(dt_res.has_value());
        CHECK(std::get<0>(*dt_res) == Catch::Approx(0.016));
    }
}

TEST_CASE("Script add_global and get_global", "[script]") {
    TestScriptEnv env;
    env.register_script("test:globals.luau", R"(
        function read_globals(): (number, string)
            return test_number, test_string
        end
    )");

    ScriptEngine engine(
        ScriptEngineConfig{.asset_manager = &env.asset_manager()});
    auto script = engine.create("test:globals.luau");
    REQUIRE(script.has_value());

    CHECK(script->add_global("test_number", 42));
    CHECK(script->add_global("test_string", std::string("serenkai")));

    auto res = script->call<int, std::string>("read_globals");
    REQUIRE(res.has_value());
    CHECK(std::get<0>(*res) == 42);
    CHECK(std::get<1>(*res) == "serenkai");

    auto global_num = script->get_global("test_number");
    REQUIRE(global_num.has_value());
    CHECK(global_num->cast<int>().value() == 42);

    auto global_str = script->get_global("test_string");
    REQUIRE(global_str.has_value());
    CHECK(global_str->cast<std::string>().value() == "serenkai");

    auto non_existent = script->get_global("non_existent_key");
    CHECK_FALSE(non_existent.has_value());
}

TEST_CASE("Script Map and Luau vector bindings", "[script][map]") {
    TestScriptEnv env;
    env.register_script("test:map_test.luau", R"(
        function get_map_dimensions(m: any): (number, number, number, number, number, number)
            local ms = m:map_size()
            local ts = m:tile_size()
            local ps = m:pixel_size()
            return ms.x, ms.y, ts.x, ts.y, ps.x, ps.y
        end

        function test_bounds(m: any, x: number, y: number): boolean
            return m:is_in_bounds(vector.create(x, y, 0))
        end

        function test_coords(m: any, wx: number, wy: number): (number, number, number, number)
            local tile = m:world_to_tile(vector.create(wx, wy, 0))
            local world = m:tile_to_world(tile)
            return tile.x, tile.y, world.x, world.y
        end

        function test_layer_visibility(m: any, layer_name: string): (boolean, boolean)
            local has = m:has_layer(layer_name)
            m:set_layer_visible(layer_name, false)
            return has, true
        end

        function test_is_solid(m: any, x: number, y: number): boolean
            return m:is_solid(vector.create(x, y, 0))
        end

        function test_check_collision(m: any, x: number, y: number, w: number, h: number): boolean
            local box = AABB.new(x, y, w, h)
            return m:check_collision(box)
        end
    )");

    ScriptEngine engine(
        ScriptEngineConfig{.asset_manager = &env.asset_manager()});
    auto script = engine.create("test:map_test.luau");
    REQUIRE(script.has_value());

    MapData data{};
    data.map_size = {10, 8};
    data.tile_size = {16, 16};
    TileLayer layer{};
    layer.name = "Ground";
    data.layers.push_back(layer);

    TileLayer col_layer{};
    col_layer.name = "collision";
    col_layer.size = {10, 8};
    col_layer.tiles.resize(80);
    col_layer.tiles[1 * 10 + 1].gid = 1; // (1, 1) is solid
    data.layers.push_back(std::move(col_layer));

    Map map(std::move(data));

    SECTION("Query map dimensions via Luau vectors") {
        auto res = script->call<double, double, double, double, double, double>(
            "get_map_dimensions", &map);
        REQUIRE(res.has_value());
        CHECK(std::get<0>(*res) == 10.0);
        CHECK(std::get<1>(*res) == 8.0);
        CHECK(std::get<2>(*res) == 16.0);
        CHECK(std::get<3>(*res) == 16.0);
        CHECK(std::get<4>(*res) == 160.0);
        CHECK(std::get<5>(*res) == 128.0);
    }

    SECTION("Check bounds using vector argument") {
        auto res1 = script->call<bool>("test_bounds", &map, 5, 4);
        REQUIRE(res1.has_value());
        CHECK(std::get<0>(*res1) == true);

        auto res2 = script->call<bool>("test_bounds", &map, 20, 20);
        REQUIRE(res2.has_value());
        CHECK(std::get<0>(*res2) == false);
    }

    SECTION("Coordinate conversion round-trip") {
        auto res = script->call<double, double, double, double>(
            "test_coords", &map, 32.0f, 48.0f);
        REQUIRE(res.has_value());
        CHECK(std::get<0>(*res) == 2.0);
        CHECK(std::get<1>(*res) == 3.0);
        CHECK(std::get<2>(*res) == 32.0);
        CHECK(std::get<3>(*res) == 48.0);
    }

    SECTION("Layer visibility operations") {
        auto res =
            script->call<bool, bool>("test_layer_visibility", &map, "Ground");
        REQUIRE(res.has_value());
        CHECK(std::get<0>(*res) == true);
    }

    SECTION("is_solid and check_collision Lua bindings") {
        auto solid_hit = script->call<bool>("test_is_solid", &map, 1, 1);
        REQUIRE(solid_hit.has_value());
        CHECK(std::get<0>(*solid_hit) == true);

        auto solid_miss = script->call<bool>("test_is_solid", &map, 0, 0);
        REQUIRE(solid_miss.has_value());
        CHECK(std::get<0>(*solid_miss) == false);

        // Tile (1, 1) is [16, 32) x [16, 32). AABB at (18, 18, 10, 10) overlaps
        // it.
        auto col_hit = script->call<bool>("test_check_collision", &map, 18.0f,
                                          18.0f, 10.0f, 10.0f);
        REQUIRE(col_hit.has_value());
        CHECK(std::get<0>(*col_hit) == true);

        // AABB at (2, 2, 10, 10) is inside empty tile (0, 0).
        auto col_miss = script->call<bool>("test_check_collision", &map, 2.0f,
                                           2.0f, 10.0f, 10.0f);
        REQUIRE(col_miss.has_value());
        CHECK(std::get<0>(*col_miss) == false);
    }
}

TEST_CASE("Script MapObject and Tile bindings", "[script][map]") {
    TestScriptEnv env;
    env.register_script("test:map_obj_test.luau", R"(
        function query_object(m: any, name: string): (string, string, number, number, boolean)
            local objs = m:find_object(name)
            if #objs == 0 then
                return "", "", 0, 0, false
            end
            local obj = objs[1]
            return obj.name, obj.type, obj.pos.x, obj.pos.y, obj.visible
        end

        function query_properties(m: any, name: string): (boolean, number, number, string, boolean)
            local objs = m:find_object(name)
            local obj = objs[1]
            local is_boss = obj:get_property("is_boss")
            local hp = obj:get_property("hp")
            local speed = obj:get_property("speed")
            local desc = obj:get_property("desc")
            local non_existent = obj:get_property("not_found")
            return is_boss, hp, speed, desc, non_existent == nil
        end

        function query_by_type(m: any, obj_type: string): number
            local objs = m:find_objects_by_type(obj_type)
            return #objs
        end

        function test_tile(t: any): (number, number, boolean)
            return t.gid, t.local_id, t:is_empty()
        end
    )");

    ScriptEngine engine(
        ScriptEngineConfig{.asset_manager = &env.asset_manager()});
    auto script = engine.create("test:map_obj_test.luau");
    REQUIRE(script.has_value());

    MapData data{};
    data.map_size = {10, 8};
    data.tile_size = {16, 16};

    ObjectGroup og{};
    og.name = "Entities";

    MapObject boss{};
    boss.name = "Boss_1";
    boss.type = "enemy";
    boss.pos = {64.0f, 128.0f};
    boss.visible = true;
    boss.properties.push_back(MapProperty{"is_boss", true});
    boss.properties.push_back(MapProperty{"hp", 500});
    boss.properties.push_back(MapProperty{"speed", 3.5f});
    boss.properties.push_back(MapProperty{"desc", std::string("Final Boss")});

    MapObject minion{};
    minion.name = "Minion_1";
    minion.type = "enemy";
    minion.pos = {32.0f, 32.0f};
    minion.visible = true;

    og.objects.push_back(std::move(boss));
    og.objects.push_back(std::move(minion));
    data.layers.push_back(std::move(og));

    Map map(std::move(data));

    SECTION("Query MapObject fields and position vector") {
        auto res = script->call<std::string, std::string, double, double, bool>(
            "query_object", &map, "Boss_1");
        REQUIRE(res.has_value());
        CHECK(std::get<0>(*res) == "Boss_1");
        CHECK(std::get<1>(*res) == "enemy");
        CHECK(std::get<2>(*res) == 64.0);
        CHECK(std::get<3>(*res) == 128.0);
        CHECK(std::get<4>(*res) == true);
    }

    SECTION("Query MapObject properties with different variant types") {
        auto res = script->call<bool, int, double, std::string, bool>(
            "query_properties", &map, "Boss_1");
        REQUIRE(res.has_value());
        CHECK(std::get<0>(*res) == true);
        CHECK(std::get<1>(*res) == 500);
        CHECK(std::get<2>(*res) == Catch::Approx(3.5));
        CHECK(std::get<3>(*res) == "Final Boss");
        CHECK(std::get<4>(*res) == true);
    }

    SECTION("Query objects by type returns list with correct size") {
        auto res = script->call<int>("query_by_type", &map, "enemy");
        REQUIRE(res.has_value());
        CHECK(std::get<0>(*res) == 2);
    }

    SECTION("Tile property and method access") {
        Tile t{};
        t.gid = 100;
        t.local_id = 15;
        auto res = script->call<int, int, bool>("test_tile", &t);
        REQUIRE(res.has_value());
        CHECK(std::get<0>(*res) == 100);
        CHECK(std::get<1>(*res) == 15);
        CHECK(std::get<2>(*res) == false);
    }
}

TEST_CASE("Script Map bindings", "[script][map]") {
    TestScriptEnv env;
    env.register_script("test:map_test.luau", R"(
        function test_switch_single(loc: string): boolean
            return World.Map.switch_map(loc)
        end

        function test_switch_double(loc: string, spawn: string): boolean
            return World.Map.switch_map(loc, spawn)
        end

        function test_has_map(): boolean
            return World.Map.has_map()
        end

        function test_current_map_nil(): boolean
            return World.Map.current_map() == nil
        end
    )");

    ScriptEngine engine(
        ScriptEngineConfig{.asset_manager = &env.asset_manager()});
    auto script = engine.create("test:map_test.luau");
    REQUIRE(script.has_value());

    MapManager manager(&env.asset_manager());
    engine.set_active_world(WorldContext{.map_mgr = &manager});

    SECTION("has_map and current_map initial state") {
        auto has = script->call<bool>("test_has_map");
        REQUIRE(has.has_value());
        CHECK(std::get<0>(*has) == false);

        auto is_nil = script->call<bool>("test_current_map_nil");
        REQUIRE(is_nil.has_value());
        CHECK(std::get<0>(*is_nil) == true);
    }

    SECTION("switch_map with single argument succeeds without type error") {
        auto res =
            script->call<bool>("test_switch_single", "nonexistent:map.tmx");
        REQUIRE(res.has_value());
        CHECK(std::get<0>(*res) == false);
    }

    SECTION("switch_map with two arguments succeeds without type error") {
        auto res = script->call<bool>("test_switch_double",
                                      "nonexistent:map.tmx", "SpawnPoint");
        REQUIRE(res.has_value());
        CHECK(std::get<0>(*res) == false);
    }

    SECTION("Fallback when world context is empty") {
        engine.set_active_world(WorldContext{});
        auto has = script->call<bool>("test_has_map");
        REQUIRE(has.has_value());
        CHECK(std::get<0>(*has) == false);
    }
}

TEST_CASE("Script Camera bindings", "[script][camera]") {
    TestScriptEnv env;
    env.register_script("test:camera_test.luau", R"(
        function get_pos(): (number, number)
            local p = World.Camera.pos()
            return p.x, p.y
        end

        function get_zoom(): number
            return World.Camera.zoom()
        end

        function set_pos(x: number, y: number)
            World.Camera.set_pos(vector.create(x, y, 0))
        end

        function set_zoom(z: number)
            World.Camera.set_zoom(z)
        end

        function add_zoom(z: number)
            World.Camera.add_zoom(z)
        end

        function move(dx: number, dy: number)
            World.Camera.move(vector.create(dx, dy, 0))
        end
    )");

    ScriptEngine engine(
        ScriptEngineConfig{.asset_manager = &env.asset_manager()});
    auto script = engine.create("test:camera_test.luau");
    REQUIRE(script.has_value());

    Camera camera;
    engine.set_active_world(WorldContext{.camera = &camera});

    SECTION("Read pos and zoom functions") {
        auto p = script->call<double, double>("get_pos");
        REQUIRE(p.has_value());
        CHECK(std::get<0>(*p) == Catch::Approx(0.0));
        CHECK(std::get<1>(*p) == Catch::Approx(0.0));

        auto z = script->call<double>("get_zoom");
        REQUIRE(z.has_value());
        CHECK(std::get<0>(*z) == Catch::Approx(3.0));
    }

    SECTION("set_pos and set_zoom via script") {
        auto set_p_res = script->call<>("set_pos", 120.0, 240.0);
        REQUIRE(set_p_res.has_value());
        CHECK(camera.pos().x == Catch::Approx(120.0f));
        CHECK(camera.pos().y == Catch::Approx(240.0f));

        auto set_z_res = script->call<>("set_zoom", 2.0);
        REQUIRE(set_z_res.has_value());
        CHECK(camera.zoom() == Catch::Approx(2.0f));
    }

    SECTION("add_zoom via script") {
        camera.set_zoom(3.0f);
        auto add_res = script->call<>("add_zoom", 2.0);
        REQUIRE(add_res.has_value());
        CHECK(camera.zoom() == Catch::Approx(5.0f));

        auto clamp_res = script->call<>("add_zoom", 50.0);
        REQUIRE(clamp_res.has_value());
        CHECK(camera.zoom() == Catch::Approx(Camera::MAX_ZOOM));
    }

    SECTION("move via script") {
        camera.set_pos({10.0f, 20.0f});
        auto move_res = script->call<>("move", 5.0, -15.0);
        REQUIRE(move_res.has_value());
        CHECK(camera.pos().x == Catch::Approx(15.0f));
        CHECK(camera.pos().y == Catch::Approx(5.0f));
    }

    SECTION("Fallback when world context is empty") {
        engine.set_active_world(WorldContext{});
        auto p = script->call<double, double>("get_pos");
        REQUIRE(p.has_value());
        CHECK(std::get<0>(*p) == Catch::Approx(0.0));
        CHECK(std::get<1>(*p) == Catch::Approx(0.0));

        auto z = script->call<double>("get_zoom");
        REQUIRE(z.has_value());
        CHECK(std::get<0>(*z) == Catch::Approx(0.0));
    }
}

TEST_CASE("Script AABB bindings", "[script][aabb]") {
    TestScriptEnv env;
    env.register_script("test:aabb_test.luau", R"(
        function test_construct_new(x: number, y: number, w: number, h: number): (number, number, number, number)
            local b = AABB.new(x, y, w, h)
            return b:left(), b:top(), b:width(), b:height()
        end

        function test_construct_call(x: number, y: number, w: number, h: number): (number, number)
            local b = AABB(x, y, w, h)
            local c = b:center()
            return c.x, c.y
        end

        function test_from_center(cx: number, cy: number, w: number, h: number): (number, number)
            local b = AABB.from_center(vector.create(cx, cy, 0), vector.create(w, h, 0))
            return b:left(), b:top()
        end

        function test_methods(): (boolean, boolean, number)
            local b1 = AABB.new(0, 0, 10, 10)
            local b2 = AABB.new(5, 5, 10, 10)
            local b3 = AABB.new(20, 20, 5, 5)
            return b1:intersects(b2), b1:intersects(b3), b1:area()
        end
    )");

    ScriptEngine engine(
        ScriptEngineConfig{.asset_manager = &env.asset_manager()});
    auto script = engine.create("test:aabb_test.luau");
    REQUIRE(script.has_value());

    SECTION("AABB.new and basic accessors") {
        auto res = script->call<double, double, double, double>(
            "test_construct_new", 10.0, 20.0, 30.0, 40.0);
        REQUIRE(res.has_value());
        CHECK(std::get<0>(*res) == Catch::Approx(10.0));
        CHECK(std::get<1>(*res) == Catch::Approx(20.0));
        CHECK(std::get<2>(*res) == Catch::Approx(30.0));
        CHECK(std::get<3>(*res) == Catch::Approx(40.0));
    }

    SECTION("AABB call constructor and center") {
        auto res = script->call<double, double>("test_construct_call", 10.0,
                                                20.0, 30.0, 40.0);
        REQUIRE(res.has_value());
        CHECK(std::get<0>(*res) == Catch::Approx(25.0));
        CHECK(std::get<1>(*res) == Catch::Approx(40.0));
    }

    SECTION("AABB.from_center") {
        auto res = script->call<double, double>("test_from_center", 50.0, 50.0,
                                                20.0, 10.0);
        REQUIRE(res.has_value());
        CHECK(std::get<0>(*res) == Catch::Approx(40.0));
        CHECK(std::get<1>(*res) == Catch::Approx(45.0));
    }

    SECTION("AABB methods intersects and area") {
        auto res = script->call<bool, bool, double>("test_methods");
        REQUIRE(res.has_value());
        CHECK(std::get<0>(*res) == true);
        CHECK(std::get<1>(*res) == false);
        CHECK(std::get<2>(*res) == Catch::Approx(100.0));
    }
}

TEST_CASE("Script EntityManager bindings", "[script][entity]") {
    TestScriptEnv env;
    env.register_script("test:entity_test.luau", R"(
        function get_null_id(): number
            return World.Entities.NULL_ID
        end

        function test_lifecycle(): (number, boolean, number, boolean, number)
            local id = World.Entities.create()
            local alive_before = World.Entities.alive(id)
            local size_before = World.Entities.alive_size()
            World.Entities.destroy(id)
            local alive_after = World.Entities.alive(id)
            local size_after = World.Entities.alive_size()
            return id, alive_before, size_before, alive_after, size_after
        end

        function test_boolean_prop(id: number): (boolean, any, boolean, any)
            local s1 = World.Entities.set_prop(id, "game:flag", true)
            local g1 = World.Entities.get_prop(id, "game:flag")
            local s2 = World.Entities.set_prop(id, "game:flag", false)
            local g2 = World.Entities.get_prop(id, "game:flag")
            return s1, g1, s2, g2
        end

        function test_number_prop(id: number): (boolean, number, boolean, number)
            local s1 = World.Entities.set_prop(id, "game:count", 42)
            local g1 = World.Entities.get_prop(id, "game:count")
            local s2 = World.Entities.set_prop(id, "game:ratio", 3.14)
            local g2 = World.Entities.get_prop(id, "game:ratio")
            return s1, g1, s2, g2
        end

        function test_string_prop(id: number): (boolean, string)
            local s = World.Entities.set_prop(id, "game:name", "hero_character")
            local g = World.Entities.get_prop(id, "game:name")
            return s, g
        end

        function test_vector_prop(id: number): (boolean, number, number, number)
            local s = World.Entities.set_prop(id, "game:velocity", vector.create(1.5, 2.5, 3.5))
            local v = World.Entities.get_prop(id, "game:velocity")
            return s, v.x, v.y, v.z
        end

        function test_has_and_remove_prop(id: number): (boolean, boolean, boolean, boolean, any)
            local has_before = World.Entities.has_prop(id, "game:score")
            World.Entities.set_prop(id, "game:score", 100)
            local has_after_set = World.Entities.has_prop(id, "game:score")
            local rm_res = World.Entities.remove_prop(id, "game:score")
            local has_after_rm = World.Entities.has_prop(id, "game:score")
            local get_after_rm = World.Entities.get_prop(id, "game:score")
            return has_before, has_after_set, rm_res, has_after_rm, get_after_rm
        end

        function test_null_entity_ops(): (boolean, boolean, boolean, any)
            local alive = World.Entities.alive(World.Entities.NULL_ID)
            local set_res = World.Entities.set_prop(World.Entities.NULL_ID, "game:prop", 123)
            local rm_res = World.Entities.remove_prop(World.Entities.NULL_ID, "game:prop")
            local get_res = World.Entities.get_prop(World.Entities.NULL_ID, "game:prop")
            return alive, set_res, rm_res, get_res
        end

        function test_create_with_loc(loc: string): number
            return World.Entities.create_with_loc(loc)
        end
    )");

    ScriptEngine engine(
        ScriptEngineConfig{.asset_manager = &env.asset_manager()});
    auto script = engine.create("test:entity_test.luau");
    REQUIRE(script.has_value());

    EntityManager manager(&env.asset_manager(), &engine);
    engine.set_active_world(WorldContext{.entity_mgr = &manager});

    SECTION("NULL_ID property") {
        auto res = script->call<double>("get_null_id");
        REQUIRE(res.has_value());
        CHECK(std::get<0>(*res) == 0.0);
    }

    SECTION("Entity creation, alive check, and destruction") {
        auto res =
            script->call<double, bool, double, bool, double>("test_lifecycle");
        REQUIRE(res.has_value());
        CHECK(std::get<0>(*res) != 0.0);
        CHECK(std::get<1>(*res) == true);
        CHECK(std::get<2>(*res) == 1.0);
        CHECK(std::get<3>(*res) == false);
        CHECK(std::get<4>(*res) == 0.0);
    }

    SECTION("Boolean property read and write") {
        EntityId e = manager.create();
        auto res = script->call<bool, bool, bool, bool>("test_boolean_prop",
                                                        static_cast<double>(e));
        REQUIRE(res.has_value());
        CHECK(std::get<0>(*res) == true);
        CHECK(std::get<1>(*res) == true);
        CHECK(std::get<2>(*res) == true);
        CHECK(std::get<3>(*res) == false);
    }

    SECTION("Number property read and write (int and float)") {
        EntityId e = manager.create();
        auto res = script->call<bool, double, bool, double>(
            "test_number_prop", static_cast<double>(e));
        REQUIRE(res.has_value());
        CHECK(std::get<0>(*res) == true);
        CHECK(std::get<1>(*res) == Catch::Approx(42.0));
        CHECK(std::get<2>(*res) == true);
        CHECK(std::get<3>(*res) == Catch::Approx(3.14));
    }

    SECTION("String property read and write") {
        EntityId e = manager.create();
        auto res = script->call<bool, std::string>("test_string_prop",
                                                   static_cast<double>(e));
        REQUIRE(res.has_value());
        CHECK(std::get<0>(*res) == true);
        CHECK(std::get<1>(*res) == "hero_character");
    }

    SECTION("Vector property read and write") {
        EntityId e = manager.create();
        auto res = script->call<bool, double, double, double>(
            "test_vector_prop", static_cast<double>(e));
        REQUIRE(res.has_value());
        CHECK(std::get<0>(*res) == true);
        CHECK(std::get<1>(*res) == Catch::Approx(1.5));
        CHECK(std::get<2>(*res) == Catch::Approx(2.5));
        CHECK(std::get<3>(*res) == Catch::Approx(3.5));
    }

    SECTION("has_prop and remove_prop operations") {
        EntityId e = manager.create();
        auto res = script->call<bool, bool, bool, bool, std::optional<int>>(
            "test_has_and_remove_prop", static_cast<double>(e));
        REQUIRE(res.has_value());
        CHECK(std::get<0>(*res) == false);
        CHECK(std::get<1>(*res) == true);
        CHECK(std::get<2>(*res) == true);
        CHECK(std::get<3>(*res) == false);
        CHECK(std::get<4>(*res).has_value() == false);
    }

    SECTION("Operations on NULL_ID return expected failure defaults") {
        auto res = script->call<bool, bool, bool, std::optional<int>>(
            "test_null_entity_ops");
        REQUIRE(res.has_value());
        CHECK(std::get<0>(*res) == false);
        CHECK(std::get<1>(*res) == false);
        CHECK(std::get<2>(*res) == false);
        CHECK(std::get<3>(*res).has_value() == false);
    }

    SECTION("Fallback when world context is empty") {
        engine.set_active_world(WorldContext{});
        auto res =
            script->call<double, bool, double, bool, double>("test_lifecycle");
        REQUIRE(res.has_value());
        CHECK(std::get<0>(*res) == 0.0);
        CHECK(std::get<1>(*res) == false);
    }

    SECTION("create_with_loc Lua binding") {
        auto res = script->call<double>("test_create_with_loc",
                                        "nonexistent:entity.json");
        REQUIRE(res.has_value());
        CHECK(std::get<0>(*res) == 0.0);
    }
}

TEST_CASE("Script Entity movement and collision",
          "[script][entity][collision]") {
    TestScriptEnv env;
    env.register_file("test:maps/collision_map.tmx",
                      R"(<?xml version="1.0" encoding="UTF-8"?>
<map version="1.10" tiledversion="1.10.2" orientation="orthogonal" renderorder="right-down" width="4" height="4" tilewidth="16" tileheight="16" infinite="0">
 <tileset firstgid="1" name="col_tileset" tilewidth="16" tileheight="16" tilecount="4" columns="2">
  <image source="col.png" width="32" height="32"/>
 </tileset>
 <layer id="1" name="collision" width="4" height="4">
  <data encoding="csv">
0,0,0,0,
0,0,1,0,
0,0,0,0,
0,0,0,0
</data>
 </layer>
</map>)",
                      ".tmx");

    env.register_script("test:move_test.luau", R"(
        function move_entity(id: number, dx: number, dy: number)
            World.Entities.move(id, vector.create(dx, dy, 0))
        end

        function get_entity_pos(id: number): (number, number, number)
            local p = World.Entities.get_prop(id, "props/pos")
            if not p then
                return -1, -1, -1
            end
            return p.x, p.y, p.z
        end
    )");

    ScriptEngine engine(
        ScriptEngineConfig{.asset_manager = &env.asset_manager()});
    auto script = engine.create("test:move_test.luau");
    REQUIRE(script.has_value());

    MapManager map_manager(&env.asset_manager());
    EntityManager entity_manager(&env.asset_manager(), &engine);
    engine.set_active_world(WorldContext{
        .map_mgr = &map_manager,
        .entity_mgr = &entity_manager,
    });

    SECTION("Entity moves freely when no collision is encountered") {
        REQUIRE(map_manager.switch_map("test:maps/collision_map.tmx"));
        EntityId e = entity_manager.create();
        entity_manager.set_prop(e, "props/pos", glm::vec3{0.0f, 0.0f, 1.0f});
        entity_manager.set_prop(e, "props/collider",
                                glm::vec4{2.0f, 2.0f, 12.0f, 12.0f});

        auto res =
            script->call<>("move_entity", static_cast<double>(e), 5.0, 6.0);
        REQUIRE(res.has_value());

        auto pos_res = script->call<double, double, double>(
            "get_entity_pos", static_cast<double>(e));
        REQUIRE(pos_res.has_value());
        CHECK(std::get<0>(*pos_res) == 5.0);
        CHECK(std::get<1>(*pos_res) == 6.0);
        CHECK(std::get<2>(*pos_res) == 1.0);
    }

    SECTION("Collision blocks movement on obstructed axis while allowing "
            "sliding on free axis") {
        REQUIRE(map_manager.switch_map("test:maps/collision_map.tmx"));
        EntityId e = entity_manager.create();
        // Tile (2, 1) is solid, world pos [32, 48) x [16, 32).
        // Place entity at x = 16.0, y = 16.0, collider = [2, 2, 12, 12].
        // Current AABB = [18, 18] -> [30, 30].
        entity_manager.set_prop(e, "props/pos", glm::vec3{16.0f, 16.0f, 1.0f});
        entity_manager.set_prop(e, "props/collider",
                                glm::vec4{2.0f, 2.0f, 12.0f, 12.0f});

        // Try to move right by +10.0 (x would be 26, collider max.x = 26 + 14 =
        // 40 > 32, hitting solid tile). Simultaneously move down by +4.0 (y
        // would be 20, collider in [22, 34], tile 2,1 is at x=32 so y is free).
        auto res =
            script->call<>("move_entity", static_cast<double>(e), 10.0, 4.0);
        REQUIRE(res.has_value());

        auto pos_res = script->call<double, double, double>(
            "get_entity_pos", static_cast<double>(e));
        REQUIRE(pos_res.has_value());
        // X movement should be blocked (stays 16.0), Y movement succeeds (16 +
        // 4 = 20.0)
        CHECK(std::get<0>(*pos_res) == 16.0);
        CHECK(std::get<1>(*pos_res) == 20.0);
    }

    SECTION("Entity without props/collider does not move") {
        REQUIRE(map_manager.switch_map("test:maps/collision_map.tmx"));
        EntityId e = entity_manager.create();
        entity_manager.set_prop(e, "props/pos", glm::vec3{0.0f, 0.0f, 1.0f});

        auto res =
            script->call<>("move_entity", static_cast<double>(e), 5.0, 5.0);
        REQUIRE(res.has_value());

        auto pos_res = script->call<double, double, double>(
            "get_entity_pos", static_cast<double>(e));
        REQUIRE(pos_res.has_value());
        CHECK(std::get<0>(*pos_res) == 0.0);
        CHECK(std::get<1>(*pos_res) == 0.0);
    }

    SECTION("move is safe when map is not loaded") {
        EntityId e = entity_manager.create();
        entity_manager.set_prop(e, "props/pos", glm::vec3{0.0f, 0.0f, 1.0f});
        entity_manager.set_prop(e, "props/collider",
                                glm::vec4{2.0f, 2.0f, 12.0f, 12.0f});

        // map_manager has no map loaded
        auto res =
            script->call<>("move_entity", static_cast<double>(e), 5.0, 5.0);
        REQUIRE(res.has_value());

        auto pos_res = script->call<double, double, double>(
            "get_entity_pos", static_cast<double>(e));
        REQUIRE(pos_res.has_value());
        CHECK(std::get<0>(*pos_res) == 0.0);
        CHECK(std::get<1>(*pos_res) == 0.0);
    }
}

TEST_CASE("ScriptEngine localization bindings", "[script]") {
    TestScriptEnv env;
    LocalizationManager loc_mgr(&env.asset_manager());

    std::string en_content = R"(msgid ""
msgstr ""
"Language: en_US\n"

msgctxt "ui:start_game"
msgid "Start"
msgstr "Start Game"

msgctxt "ui:greeting"
msgid "Greeting"
msgstr "Hello, {name}!"

msgctxt "ui:stats"
msgid "Stats"
msgstr "Level: {level}, HP: {hp}"
)";

    std::string zh_content = R"(msgid ""
msgstr ""
"Language: zh_CN\n"

msgctxt "ui:start_game"
msgid "Start"
msgstr "开始游戏"

msgctxt "ui:greeting"
msgid "Greeting"
msgstr "你好，{name}！"
)";

    env.register_file("test:locales/en.po", en_content, ".po");
    env.register_file("test:locales/zh.po", zh_content, ".po");

    REQUIRE(loc_mgr.load_language(Language::en_US, "test:locales/en.po"));

    ScriptEngine engine(ScriptEngineConfig{
        .asset_manager = &env.asset_manager(),
        .localization_manager = &loc_mgr,
    });

    SECTION("Tr translates keys without arguments and with table arguments") {
        env.register_script("test:tr_test.luau", R"(
            function translate_simple()
                return Tr("ui:start_game")
            end

            function translate_missing()
                return Tr("ui:unknown_key")
            end

            function translate_with_args()
                return Tr("ui:greeting", { name = "Alice" })
            end

            function translate_with_multiple_args()
                return Tr("ui:stats", { level = "10", hp = "100" })
            end
        )");

        auto script = engine.create("test:tr_test.luau");
        REQUIRE(script.has_value());

        auto res_simple = script->call<std::string>("translate_simple");
        REQUIRE(res_simple.has_value());
        CHECK(std::get<0>(*res_simple) == "Start Game");

        auto res_missing = script->call<std::string>("translate_missing");
        REQUIRE(res_missing.has_value());
        CHECK(std::get<0>(*res_missing) == "ui:unknown_key");

        auto res_args = script->call<std::string>("translate_with_args");
        REQUIRE(res_args.has_value());
        CHECK(std::get<0>(*res_args) == "Hello, Alice!");

        auto res_multi =
            script->call<std::string>("translate_with_multiple_args");
        REQUIRE(res_multi.has_value());
        CHECK(std::get<0>(*res_multi) == "Level: 10, HP: 100");
    }

    SECTION("Loc controls language, fallback and dynamic loading") {
        env.register_script("test:loc_test.luau", R"(
            function get_current()
                return Loc.current()
            end

            function get_fallback()
                return Loc.fallback()
            end

            function switch_to_zh()
                return Loc.set("zh_CN")
            end

            function switch_to_invalid()
                return Loc.set("invalid_lang")
            end

            function set_fallback_en()
                return Loc.set_fallback("en_US")
            end

            function set_fallback_invalid()
                return Loc.set_fallback("not_a_language")
            end

            function load_zh()
                return Loc.load("zh_CN", "test:locales/zh.po")
            end

            function load_invalid()
                return Loc.load("not_valid", "test:locales/zh.po")
            end

            function check_has(key)
                return Loc.has(key)
            end
        )");

        auto script = engine.create("test:loc_test.luau");
        REQUIRE(script.has_value());

        auto cur = script->call<std::string>("get_current");
        REQUIRE(cur.has_value());
        CHECK(std::get<0>(*cur) == "en_US");

        auto fb = script->call<std::string>("get_fallback");
        REQUIRE(fb.has_value());
        CHECK(std::get<0>(*fb) == "en_US");

        auto has_key = script->call<bool>("check_has", "ui:start_game");
        REQUIRE(has_key.has_value());
        CHECK(std::get<0>(*has_key) == true);

        auto has_none = script->call<bool>("check_has", "ui:nonexistent");
        REQUIRE(has_none.has_value());
        CHECK(std::get<0>(*has_none) == false);

        // Load zh_CN from Lua
        auto load_bad = script->call<bool>("load_invalid");
        REQUIRE(load_bad.has_value());
        CHECK(std::get<0>(*load_bad) == false);

        auto load_ok = script->call<bool>("load_zh");
        REQUIRE(load_ok.has_value());
        CHECK(std::get<0>(*load_ok) == true);

        // Switch language
        auto set_bad = script->call<bool>("switch_to_invalid");
        REQUIRE(set_bad.has_value());
        CHECK(std::get<0>(*set_bad) == false);

        auto set_ok = script->call<bool>("switch_to_zh");
        REQUIRE(set_ok.has_value());
        CHECK(std::get<0>(*set_ok) == true);

        auto cur_after = script->call<std::string>("get_current");
        REQUIRE(cur_after.has_value());
        CHECK(std::get<0>(*cur_after) == "zh_CN");

        // Verify translation updated
        env.register_script("test:tr_zh.luau", R"(
            function tr_zh()
                return Tr("ui:start_game")
            end
        )");
        auto script_zh = engine.create("test:tr_zh.luau");
        REQUIRE(script_zh.has_value());
        auto res_zh = script_zh->call<std::string>("tr_zh");
        REQUIRE(res_zh.has_value());
        CHECK(std::get<0>(*res_zh) == "开始游戏");

        // Fallback setting
        auto fb_bad = script->call<bool>("set_fallback_invalid");
        REQUIRE(fb_bad.has_value());
        CHECK(std::get<0>(*fb_bad) == false);

        auto fb_ok = script->call<bool>("set_fallback_en");
        REQUIRE(fb_ok.has_value());
        CHECK(std::get<0>(*fb_ok) == true);

        auto fb_after = script->call<std::string>("get_fallback");
        REQUIRE(fb_after.has_value());
        CHECK(std::get<0>(*fb_after) == "en_US");
    }

    SECTION("Engine behaves safely when LocalizationManager is nullptr") {
        ScriptEngine null_loc_engine(ScriptEngineConfig{
            .asset_manager = &env.asset_manager(),
            .localization_manager = nullptr,
        });
        env.register_script("test:no_loc.luau", R"(
            function check_globals()
                return Tr == nil and Loc == nil
            end
        )");
        auto script = null_loc_engine.create("test:no_loc.luau");
        REQUIRE(script.has_value());
        auto res = script->call<bool>("check_globals");
        REQUIRE(res.has_value());
        CHECK(std::get<0>(*res) == true);
    }
}
