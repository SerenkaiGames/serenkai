#include "serenkai/application/event.hpp"
#include "serenkai/base/raii.hpp"
#include "serenkai/gui/button.hpp"
#include "serenkai/gui/gui_context.hpp"
#include "serenkai/gui/rect.hpp"
#include "serenkai/gui/ui_manager.hpp"
#include "serenkai/resource/asset_manager.hpp"
#include "serenkai/resource/directory_source.hpp"
#include "serenkai/resource/font_manager.hpp"
#include "serenkai/resource/texture_manager.hpp"
#include "serenkai/script/script_engine.hpp"

#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include <fstream>
#include <string_view>

namespace fs = std::filesystem;
using namespace serenkai;

namespace {

/// @brief Helper to write text content to a file.
void write_test_file(const fs::path& path, std::string_view content) {
    std::ofstream file(path);
    REQUIRE(file.is_open());
    file << content;
}

/// @brief Helper context for UiManager unit tests.
struct TestUiContext {
    AssetManager asset_manager;
    FontManager font_manager;
    TextureManager texture_manager;
    ScriptEngine script_engine;
    UiManager ui_manager;

    explicit TestUiContext(const fs::path& dir,
                           SDL_Renderer* renderer = nullptr)
        : font_manager(&asset_manager),
          texture_manager(&asset_manager, renderer),
          script_engine(ScriptEngineConfig{&asset_manager}),
          ui_manager(WidgetParserConfig{&asset_manager, &font_manager,
                                        &texture_manager, nullptr,
                                        &script_engine}) {
        asset_manager.merge_source(std::make_shared<DirectorySource>(dir));
    }
};

} // namespace

TEST_CASE("UiManager deferred operation queue", "[gui][ui_manager]") {
    fs::path temp_dir = fs::temp_directory_path() / "serenkai_test_ui_manager";
    fs::remove_all(temp_dir);
    fs::create_directories(temp_dir / "ui");
    RaiiGuard cleanup([]() {}, [&temp_dir]() { fs::remove_all(temp_dir); });

    write_test_file(temp_dir / "assets.json", R"({"ns": "test"})");

    write_test_file(temp_dir / "ui" / "view_a.json", R"({
        "view_a_rect": {
            "type": "rect",
            "size": [100, 50]
        }
    })");

    write_test_file(temp_dir / "ui" / "view_b.json", R"({
        "view_b_rect": {
            "type": "rect",
            "size": [200, 100]
        }
    })");

    TestUiContext ctx(temp_dir);

    SECTION("Operations are deferred until update is called") {
        ctx.ui_manager.request_open("test:ui/view_a.json");

        // Before update(), view should not consume events
        MouseMoveEvent move_ev{10.0f, 10.0f, 0.0f, 0.0f};
        CHECK_FALSE(ctx.ui_manager.handle_event(Event{move_ev}));

        // After update(), operation is processed
        ctx.ui_manager.update(0.016f);
    }

    SECTION("Duplicate open requests are prevented") {
        ctx.ui_manager.request_open("test:ui/view_a.json");
        ctx.ui_manager.request_open("test:ui/view_a.json");
        ctx.ui_manager.update(0.016f);

        // Closing once should remove the view
        ctx.ui_manager.request_close("test:ui/view_a.json");
        ctx.ui_manager.update(0.016f);

        // Subsequent close is a safe no-op
        ctx.ui_manager.request_close("test:ui/view_a.json");
        ctx.ui_manager.update(0.016f);
    }

    SECTION("Clear removes all views") {
        ctx.ui_manager.request_open("test:ui/view_a.json");
        ctx.ui_manager.request_open("test:ui/view_b.json");
        ctx.ui_manager.update(0.016f);

        ctx.ui_manager.request_clear();
        ctx.ui_manager.update(0.016f);

        MouseMoveEvent move_ev{10.0f, 10.0f, 0.0f, 0.0f};
        CHECK_FALSE(ctx.ui_manager.handle_event(Event{move_ev}));
    }

    SECTION("Invalid resource location is safely rejected") {
        ctx.ui_manager.request_open("invalid_uri_without_colon");
        ctx.ui_manager.request_close("invalid_uri_without_colon");
        ctx.ui_manager.update(0.016f);
    }
}

TEST_CASE("UiManager button callback registration and dispatch",
          "[gui][ui_manager]") {
    fs::path temp_dir =
        fs::temp_directory_path() / "serenkai_test_ui_manager_btn";
    fs::remove_all(temp_dir);
    fs::create_directories(temp_dir / "ui");
    RaiiGuard cleanup([]() {}, [&temp_dir]() { fs::remove_all(temp_dir); });

    write_test_file(temp_dir / "assets.json", R"({"ns": "test"})");

    write_test_file(temp_dir / "ui" / "btn_view.json", R"({
        "click_btn": {
            "type": "button",
            "callback": "on_btn_clicked",
            "children": {
                "btn_box": {
                    "type": "rect",
                    "size": [80, 30]
                }
            }
        }
    })");

    TestUiContext ctx(temp_dir);
    int callback_invocations = 0;

    ctx.ui_manager.register_callback("on_btn_clicked",
                                     [&]() { ++callback_invocations; });

    ctx.ui_manager.request_open("test:ui/btn_view.json");
    ctx.ui_manager.update(0.016f);

    SECTION("Clicking button invokes registered callback") {
        MouseMoveEvent move_in{10.0f, 10.0f, 0.0f, 0.0f};
        ctx.ui_manager.handle_event(Event{move_in});

        KeyEvent left_click{Key::MouseLeft, KeyAction::Press};
        CHECK(ctx.ui_manager.handle_event(Event{left_click}));
        CHECK(callback_invocations == 1);
    }

    SECTION("Reloading view preserves callback binding") {
        ctx.ui_manager.request_reload_all();
        ctx.ui_manager.update(0.016f);

        MouseMoveEvent move_in{10.0f, 10.0f, 0.0f, 0.0f};
        ctx.ui_manager.handle_event(Event{move_in});

        KeyEvent left_click{Key::MouseLeft, KeyAction::Press};
        CHECK(ctx.ui_manager.handle_event(Event{left_click}));
        CHECK(callback_invocations == 1);
    }
}
