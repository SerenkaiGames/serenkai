#include "serenkai/application/event.hpp"
#include "serenkai/application/input_manager.hpp"
#include "serenkai/application/key.hpp"
#include "serenkai/resource/asset_manager.hpp"
#include "serenkai/resource/asset_source.hpp"
#include "serenkai/resource/resource_location.hpp"
#include "serenkai/script/script.hpp"
#include "serenkai/script/script_engine.hpp"

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

class MockInputScriptSource : public AssetSource {
public:
    explicit MockInputScriptSource(AssetFileMap files)
        : m_files(std::move(files)) {}

    AssetFileMap& get_asset_files() override { return m_files; }
    std::string source_name() const override { return "MockInputScriptSource"; }

private:
    AssetFileMap m_files;
};

class TestInputScriptEnv {
public:
    TestInputScriptEnv() {
        auto timestamp =
            std::chrono::steady_clock::now().time_since_epoch().count();
        m_temp_dir = fs::temp_directory_path() /
                     ("serenkai_test_input_" + std::to_string(timestamp));
        fs::create_directories(m_temp_dir);
    }

    ~TestInputScriptEnv() {
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
            std::make_shared<MockInputScriptSource>(std::move(files)));
    }

    AssetManager& asset_manager() { return m_asset_manager; }

private:
    fs::path m_temp_dir;
    AssetManager m_asset_manager;
    size_t m_counter = 0;
};

} // namespace

TEST_CASE("InputManager config merging and action mapping", "[input_manager]") {
    InputManager mgr;

    SECTION("Merge valid config maps actions and keys correctly") {
        constexpr std::string_view json = R"({
            "bindings": {
                "inputs/move_up": ["W", "Up"],
                "inputs/jump": ["Space"]
            }
        })";

        REQUIRE(mgr.merge_config(json));

        const auto& action_map = mgr.get_action_id_map();
        REQUIRE(action_map.contains("inputs/move_up"));
        REQUIRE(action_map.contains("inputs/jump"));

        auto up_id = action_map.at("inputs/move_up");
        auto jump_id = action_map.at("inputs/jump");

        CHECK(up_id != jump_id);
        CHECK(mgr.get_action_name(up_id) == "inputs/move_up");
        CHECK(mgr.get_action_name(jump_id) == "inputs/jump");
    }

    SECTION("Invalid JSON fails gracefully") {
        CHECK_FALSE(mgr.merge_config("invalid json"));
        CHECK_FALSE(mgr.merge_config(R"({"bindings": "not_an_object"})"));
    }

    SECTION("Incremental merging appends new actions and keeps existing IDs") {
        constexpr std::string_view json1 = R"({
            "bindings": {
                "inputs/move_up": ["W"]
            }
        })";
        constexpr std::string_view json2 = R"({
            "bindings": {
                "inputs/move_up": ["Up"],
                "inputs/move_down": ["S"]
            }
        })";

        REQUIRE(mgr.merge_config(json1));
        auto up_id_1 = mgr.get_action_id_map().at("inputs/move_up");

        REQUIRE(mgr.merge_config(json2));
        auto up_id_2 = mgr.get_action_id_map().at("inputs/move_up");
        CHECK(up_id_1 == up_id_2);
        CHECK(mgr.get_action_id_map().contains("inputs/move_down"));
    }
}

TEST_CASE("InputManager key event lifecycle and reference counting",
          "[input_manager]") {
    InputManager mgr;
    constexpr std::string_view json = R"({
        "bindings": {
            "inputs/move_up": ["W", "Up"]
        }
    })";
    REQUIRE(mgr.merge_config(json));
    auto up_id = mgr.get_action_id_map().at("inputs/move_up");

    SECTION("Single key press, tick, and release cycle") {
        CHECK_FALSE(mgr.is_down(up_id));
        CHECK_FALSE(mgr.is_pressed(up_id));
        CHECK_FALSE(mgr.is_released(up_id));
        CHECK(mgr.get_held_actions().empty());

        // Press W
        mgr.process_key_event(KeyEvent{Key::W, KeyAction::Press});
        CHECK(mgr.is_down(up_id));
        CHECK(mgr.is_pressed(up_id));
        CHECK_FALSE(mgr.is_released(up_id));
        REQUIRE(mgr.get_held_actions().size() == 1);
        CHECK(mgr.get_held_actions()[0] == up_id);

        // Tick to next frame
        mgr.tick();
        CHECK(mgr.is_down(up_id));
        CHECK_FALSE(mgr.is_pressed(up_id));
        CHECK_FALSE(mgr.is_released(up_id));

        // Release W
        mgr.process_key_event(KeyEvent{Key::W, KeyAction::Release});
        CHECK_FALSE(mgr.is_down(up_id));
        CHECK_FALSE(mgr.is_pressed(up_id));
        CHECK(mgr.is_released(up_id));
        CHECK(mgr.get_held_actions().empty());

        // Tick clears released state
        mgr.tick();
        CHECK_FALSE(mgr.is_down(up_id));
        CHECK_FALSE(mgr.is_pressed(up_id));
        CHECK_FALSE(mgr.is_released(up_id));
    }

    SECTION("Key repeat events are ignored") {
        mgr.process_key_event(KeyEvent{Key::W, KeyAction::Press});
        mgr.tick();
        CHECK(mgr.is_down(up_id));
        CHECK_FALSE(mgr.is_pressed(up_id));

        mgr.process_key_event(KeyEvent{Key::W, KeyAction::Repeat});
        CHECK(mgr.is_down(up_id));
        CHECK_FALSE(mgr.is_pressed(up_id));
        CHECK_FALSE(mgr.is_released(up_id));
    }

    SECTION("Multi-key binding keeps action held until all keys released") {
        // Press W
        mgr.process_key_event(KeyEvent{Key::W, KeyAction::Press});
        CHECK(mgr.is_down(up_id));
        CHECK(mgr.is_pressed(up_id));

        mgr.tick();

        // Press Up as well
        mgr.process_key_event(KeyEvent{Key::Up, KeyAction::Press});
        CHECK(mgr.is_down(up_id));
        CHECK_FALSE(mgr.is_pressed(up_id));

        // Release W: action should still be held because Up is still down
        mgr.process_key_event(KeyEvent{Key::W, KeyAction::Release});
        CHECK(mgr.is_down(up_id));
        CHECK_FALSE(mgr.is_released(up_id));

        // Release Up: now action is fully released
        mgr.process_key_event(KeyEvent{Key::Up, KeyAction::Release});
        CHECK_FALSE(mgr.is_down(up_id));
        CHECK(mgr.is_released(up_id));
    }

    SECTION("Unbound keys do not affect state") {
        mgr.process_key_event(KeyEvent{Key::Space, KeyAction::Press});
        CHECK_FALSE(mgr.is_down(up_id));
        CHECK_FALSE(mgr.is_pressed(up_id));
        CHECK(mgr.get_held_actions().empty());
    }
}

TEST_CASE("InputManager boundary and query safety", "[input_manager]") {
    InputManager mgr;
    constexpr std::string_view json = R"({
        "bindings": {
            "inputs/action_0": ["A"]
        }
    })";
    REQUIRE(mgr.merge_config(json));

    // Valid registered ID
    CHECK(mgr.get_action_name(0) == "inputs/action_0");
    CHECK_FALSE(mgr.is_down(0));
    CHECK_FALSE(mgr.is_pressed(0));
    CHECK_FALSE(mgr.is_released(0));

    // Max valid boundary (MAX_ACTIONS - 1)
    CHECK_FALSE(mgr.is_down(InputManager::MAX_ACTIONS - 1));
    CHECK_FALSE(mgr.is_pressed(InputManager::MAX_ACTIONS - 1));
    CHECK_FALSE(mgr.is_released(InputManager::MAX_ACTIONS - 1));
}

TEST_CASE("InputManager Lua script bindings", "[input_manager][script]") {
    InputManager input_mgr;
    constexpr std::string_view json = R"({
        "bindings": {
            "inputs/fire": ["Space"]
        }
    })";
    REQUIRE(input_mgr.merge_config(json));

    TestInputScriptEnv env;
    env.register_script("test:input_script.luau", R"(
        function get_fire_action_id(): number
            return Input.Action["inputs/fire"]
        end

        function is_fire_down(): boolean
            return Input.is_down(Input.Action["inputs/fire"])
        end

        function is_fire_pressed(): boolean
            return Input.is_pressed(Input.Action["inputs/fire"])
        end

        function count_held_actions(): number
            local held = Input.get_held_actions()
            return #held
        end
    )");

    ScriptEngine engine(ScriptEngineConfig{
        .asset_manager = &env.asset_manager(),
        .input_manager = &input_mgr,
    });

    auto script = engine.create("test:input_script.luau");
    REQUIRE(script.has_value());

    SECTION("Lua reads action ID and detects initial unpressed state") {
        auto id_res = script->call<int>("get_fire_action_id");
        REQUIRE(id_res.has_value());
        CHECK(std::get<0>(*id_res) ==
              input_mgr.get_action_id_map().at("inputs/fire"));

        auto down_res = script->call<bool>("is_fire_down");
        REQUIRE(down_res.has_value());
        CHECK(std::get<0>(*down_res) == false);

        auto count_res = script->call<int>("count_held_actions");
        REQUIRE(count_res.has_value());
        CHECK(std::get<0>(*count_res) == 0);
    }

    SECTION("Lua detects action press and held actions list") {
        input_mgr.process_key_event(KeyEvent{Key::Space, KeyAction::Press});

        auto down_res = script->call<bool>("is_fire_down");
        REQUIRE(down_res.has_value());
        CHECK(std::get<0>(*down_res) == true);

        auto pressed_res = script->call<bool>("is_fire_pressed");
        REQUIRE(pressed_res.has_value());
        CHECK(std::get<0>(*pressed_res) == true);

        auto count_res = script->call<int>("count_held_actions");
        REQUIRE(count_res.has_value());
        CHECK(std::get<0>(*count_res) == 1);
    }
}
