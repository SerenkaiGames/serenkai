#include "serenkai/resource/asset_manager.hpp"
#include "serenkai/resource/asset_source.hpp"
#include "serenkai/resource/resource_location.hpp"
#include "serenkai/script/script_engine.hpp"

#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <string_view>
#include <tuple>
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

        auto file_name =
            std::string(loc->ns) + "_" + std::to_string(m_counter++) + ".luau";
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

private:
    fs::path m_temp_dir;
    AssetManager m_asset_manager;
    size_t m_counter = 0;
};

} // namespace

TEST_CASE("ScriptEngine script loading and lifecycle", "[script]") {
    TestScriptEnv env;
    env.register_script("test:valid.luau",
                        "function test_func() return 42 end");
    env.register_script("test:syntax_err.luau",
                        "this is invalid lua code !@#$%");
    env.register_script("test:runtime_err.luau",
                        "error('runtime error on load')");

    ScriptEngine engine(&env.asset_manager());

    SECTION("Load valid script succeeds") {
        CHECK(engine.load("test:valid.luau"));
    }

    SECTION("Duplicate load returns false") {
        CHECK(engine.load("test:valid.luau"));
        CHECK_FALSE(engine.load("test:valid.luau"));
    }

    SECTION("Load invalid resource location returns false") {
        CHECK_FALSE(engine.load(""));
        CHECK_FALSE(engine.load("invalid::loc"));
    }

    SECTION("Load non-existent asset returns false") {
        CHECK_FALSE(engine.load("test:non_existent.luau"));
    }

    SECTION("Load script with syntax error returns false") {
        CHECK_FALSE(engine.load("test:syntax_err.luau"));
    }

    SECTION("Load script with runtime error on execution returns false") {
        CHECK_FALSE(engine.load("test:runtime_err.luau"));
    }

    SECTION("Load with null AssetManager returns false") {
        ScriptEngine null_engine(nullptr);
        CHECK_FALSE(null_engine.load("test:valid.luau"));
    }

    SECTION("Unload loaded script succeeds and allows reload") {
        CHECK(engine.load("test:valid.luau"));
        CHECK(engine.unload("test:valid.luau"));
        CHECK_FALSE(engine.unload("test:valid.luau"));
        CHECK(engine.load("test:valid.luau"));
    }

    SECTION("Unload non-existent script returns false") {
        CHECK_FALSE(engine.unload("test:not_loaded.luau"));
    }
}

TEST_CASE("ScriptEngine function call invocation", "[script]") {
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
    REQUIRE(engine.load("test:math.luau"));

    SECTION("Call function with arguments and single return value") {
        auto res = engine.call<int>("test:math.luau", "add", 15, 27);
        REQUIRE(res.has_value());
        CHECK(std::get<0>(*res) == 42);
    }

    SECTION("Call function with string argument and string return value") {
        auto res =
            engine.call<std::string>("test:math.luau", "greet", "serenkai");
        REQUIRE(res.has_value());
        CHECK(std::get<0>(*res) == "hello serenkai");
    }

    SECTION("Call function with multiple return values") {
        auto res = engine.call<int, std::string>("test:math.luau", "get_pair");
        REQUIRE(res.has_value());
        CHECK(std::get<0>(*res) == 100);
        CHECK(std::get<1>(*res) == "player");
    }

    SECTION("Call function with no return values") {
        auto res = engine.call<>("test:math.luau", "no_return");
        CHECK(res.has_value());
    }

    SECTION("Call non-existent function returns nullopt") {
        auto res = engine.call<int>("test:math.luau", "non_existent");
        CHECK_FALSE(res.has_value());
    }

    SECTION("Call non-callable global returns nullopt") {
        auto res = engine.call<int>("test:math.luau", "not_a_func");
        CHECK_FALSE(res.has_value());
    }

    SECTION("Call function in unloaded script returns nullopt") {
        auto res = engine.call<int>("test:not_loaded.luau", "add", 1, 2);
        CHECK_FALSE(res.has_value());
    }

    SECTION("Call function that errors returns nullopt") {
        auto res = engine.call<>("test:math.luau", "failing_func");
        CHECK_FALSE(res.has_value());
    }
}

TEST_CASE("ScriptEngine per-resource thread isolation", "[script]") {
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
    REQUIRE(engine.load("test:script_a.luau"));
    REQUIRE(engine.load("test:script_b.luau"));

    SECTION("Globals in different script threads are isolated") {
        auto res_a = engine.call<std::string>("test:script_a.luau", "get_val");
        auto res_b = engine.call<std::string>("test:script_b.luau", "get_val");

        REQUIRE(res_a.has_value());
        REQUIRE(res_b.has_value());
        CHECK(std::get<0>(*res_a) == "value_from_a");
        CHECK(std::get<0>(*res_b) == "value_from_b");
    }

    SECTION("Calling function after unloading fails") {
        CHECK(engine.unload("test:script_a.luau"));
        auto res_a = engine.call<std::string>("test:script_a.luau", "get_val");
        CHECK_FALSE(res_a.has_value());

        // script_b is still accessible
        auto res_b = engine.call<std::string>("test:script_b.luau", "get_val");
        REQUIRE(res_b.has_value());
        CHECK(std::get<0>(*res_b) == "value_from_b");
    }
}

TEST_CASE("ScriptEngine logging bindings and standard libraries", "[script]") {
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
    REQUIRE(engine.load("test:logging.luau"));

    SECTION("Logging functions execute without error") {
        auto res = engine.call<bool>("test:logging.luau", "test_logging");
        REQUIRE(res.has_value());
        CHECK(std::get<0>(*res) == true);
    }

    SECTION("Standard library functions work in sandboxed thread") {
        auto res =
            engine.call<std::string, int>("test:logging.luau", "test_stdlib");
        REQUIRE(res.has_value());
        CHECK(std::get<0>(*res) == "HELLO");
        CHECK(std::get<1>(*res) == 20);
    }
}
