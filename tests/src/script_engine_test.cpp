#include "serenkai/script/script_engine.hpp"

#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include <fstream>
#include <string>

namespace fs = std::filesystem;
using namespace serenkai;

TEST_CASE("ScriptEngine basic execution and lifecycle", "[script]") {
    ScriptEngine engine;

    SECTION("Execute simple arithmetic expression") {
        CHECK(engine.run_string("local a = 1 + 2"));
    }

    SECTION("Execute Luau typed syntax") {
        CHECK(engine.run_string(
            "local x: number = 42\n"
            "local s: string = 'hello'\n"
            "local function add(a: number, b: number): number\n"
            "    return a + b\n"
            "end\n"
            "local res = add(x, 10)\n"));
    }

    SECTION("State persistence across run_string calls") {
        CHECK(engine.run_string("global_var = 100", "setup"));
        CHECK(engine.run_string("assert(global_var == 100)", "verify"));
    }
}

TEST_CASE("ScriptEngine error handling", "[script]") {
    ScriptEngine engine;

    SECTION("Syntax / compile error returns false") {
        CHECK_FALSE(engine.run_string("this is not valid lua code !@#$%",
                                      "bad_syntax"));
    }

    SECTION("Runtime error via error() returns false") {
        CHECK_FALSE(engine.run_string("error('deliberate runtime error')",
                                      "runtime_err"));
    }

    SECTION("Calling nil as a function returns false") {
        CHECK_FALSE(engine.run_string("local f = nil; f()", "call_nil"));
    }

    SECTION("Assert failure returns false") {
        CHECK_FALSE(engine.run_string("assert(false, 'assertion failed')",
                                      "assert_fail"));
    }
}

TEST_CASE("ScriptEngine logging bindings", "[script]") {
    ScriptEngine engine;

    SECTION("Call print with various argument types") {
        CHECK(engine.run_string("print('Hello from Luau!')"));
        CHECK(engine.run_string(
            "print('Number:', 123, 'Bool:', true, 'Nil:', nil)"));
    }

    SECTION("Call log namespace functions") {
        CHECK(engine.run_string("log.info('Info level message')"));
        CHECK(engine.run_string("log.warn('Warning level message')"));
        CHECK(engine.run_string("log.error('Error level message')"));
        CHECK(engine.run_string("log.debug('Debug level message')"));
    }

    SECTION("Call log with multiple arguments") {
        CHECK(engine.run_string("log.info('Score:', 999, 'Status:', 'OK')"));
    }
}

TEST_CASE("ScriptEngine run_file execution", "[script]") {
    ScriptEngine engine;

    SECTION("Non-existent file returns false") {
        CHECK_FALSE(engine.run_file("non_existent_script_file_12345.lua"));
    }

    SECTION("Valid file execution") {
        auto temp_dir = fs::temp_directory_path();
        auto temp_file = temp_dir / "serenkai_test_valid.lua";

        {
            std::ofstream out(temp_file);
            out << "file_executed = true\n";
            out << "print('Script file executed successfully')\n";
        }

        CHECK(engine.run_file(temp_file.string()));
        CHECK(engine.run_string("assert(file_executed == true)"));

        fs::remove(temp_file);
    }

    SECTION("Invalid file syntax returns false") {
        auto temp_dir = fs::temp_directory_path();
        auto temp_file = temp_dir / "serenkai_test_invalid.lua";

        {
            std::ofstream out(temp_file);
            out << "invalid syntax ???\n";
        }

        CHECK_FALSE(engine.run_file(temp_file.string()));

        fs::remove(temp_file);
    }
}
