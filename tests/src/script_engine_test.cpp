#include "serenkai/resource/asset_manager.hpp"
#include "serenkai/resource/asset_source.hpp"
#include "serenkai/resource/directory_source.hpp"
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

    void register_script(std::string_view loc_str, const std::string& content) {
        auto loc = ResourceLocation::parse(loc_str);
        if (!loc) {
            return;
        }

        auto file_name = std::string(loc->ns()) + "_" +
                         std::to_string(m_counter++) + ".luau";
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

    AssetManager& asset_manager() { return m_asset_manager; }

    /// @brief Merges the project assets directory into the asset manager.
    void merge_project_assets() {
        fs::path asset_dir;
#ifdef SERENKAI_TEST_ASSET_DIR
        if (fs::exists(SERENKAI_TEST_ASSET_DIR)) {
            asset_dir = SERENKAI_TEST_ASSET_DIR;
        }
#endif
        if (asset_dir.empty()) {
            if (fs::exists("assets")) {
                asset_dir = "assets";
            } else if (fs::exists("../../assets")) {
                asset_dir = "../../assets";
            }
        }
        if (!asset_dir.empty()) {
            m_asset_manager.merge_source(
                std::make_shared<DirectorySource>(asset_dir));
        }
    }

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

    ScriptEngine engine(&env.asset_manager());

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
        ScriptEngine null_engine(nullptr);
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

    ScriptEngine engine(&env.asset_manager());
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

    ScriptEngine engine(&env.asset_manager());
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

    ScriptEngine engine(&env.asset_manager());
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

    ScriptEngine engine(&env.asset_manager());

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
    env.merge_project_assets();

    ScriptEngine engine(&env.asset_manager());

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

    SECTION("main.luau on_update dispatches serenkai:events/update") {
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
        auto main = engine.create("serenkai:scripts/main.luau");
        REQUIRE(listener.has_value());
        REQUIRE(main.has_value());

        // Trigger on_update through main.luau
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

    ScriptEngine engine(&env.asset_manager());
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
