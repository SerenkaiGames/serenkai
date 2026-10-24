#include "serenkai/application/event.hpp"
#include "serenkai/base/concepts.hpp"
#include "serenkai/base/raii.hpp"
#include "serenkai/base/type_name.hpp"
#include "serenkai/gui/anchor.hpp"
#include "serenkai/gui/button.hpp"
#include "serenkai/gui/color.hpp"
#include "serenkai/gui/column_layout.hpp"
#include "serenkai/gui/image_widget.hpp"
#include "serenkai/gui/label.hpp"
#include "serenkai/gui/rect.hpp"
#include "serenkai/gui/ui_view.hpp"
#include "serenkai/gui/widget_parser.hpp"
#include "serenkai/resource/asset_manager.hpp"
#include "serenkai/resource/directory_source.hpp"
#include "serenkai/resource/font_manager.hpp"
#include "serenkai/resource/texture_manager.hpp"
#include "serenkai/script/script_engine.hpp"

#include <SDL3/SDL.h>
#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include <fstream>
#include <memory>
#include <stb_image_write.h>
#include <string>

namespace fs = std::filesystem;
using namespace serenkai;

namespace {

/// @brief Helper to write text content to a file.
void write_file(const fs::path& path, std::string_view content) {
    std::ofstream file(path);
    REQUIRE(file.is_open());
    file << content;
}

/// @brief Helper structure containing test parser and its dependencies.
struct TestParserContext {
    AssetManager asset_manager;
    FontManager font_manager;
    TextureManager texture_manager;
    ScriptEngine script_engine;
    WidgetParser parser;

    explicit TestParserContext(const fs::path& dir,
                               SDL_Renderer* renderer = nullptr)
        : font_manager(&asset_manager),
          texture_manager(&asset_manager, renderer),
          script_engine(ScriptEngineConfig{&asset_manager}),
          parser(WidgetParserConfig{&asset_manager, &font_manager,
                                    &texture_manager, nullptr,
                                    &script_engine}) {
        asset_manager.merge_source(std::make_shared<DirectorySource>(dir));
    }
};

} // namespace

TEST_CASE("PlainType concept validation", "[base][concepts]") {
    STATIC_CHECK(PlainType<int>);
    STATIC_CHECK(PlainType<std::string>);
    STATIC_CHECK(PlainType<glm::ivec2>);

    STATIC_CHECK_FALSE(PlainType<int&>);
    STATIC_CHECK_FALSE(PlainType<const int>);
    STATIC_CHECK_FALSE(PlainType<int*>);
    STATIC_CHECK_FALSE(PlainType<const std::string&>);
}

TEST_CASE("type_name helper utility", "[base][type_name]") {
    CHECK(type_name<int>() == "int");
    CHECK(type_name<float>() == "float");
    CHECK(type_name<double>() == "double");
}

TEST_CASE("WidgetParser label construction and property parsing",
          "[gui][parser]") {
    fs::path temp_dir = fs::temp_directory_path() / "serenkai_test_parser_prop";
    fs::remove_all(temp_dir);
    fs::create_directories(temp_dir / "ui");
    RaiiGuard cleanup([]() {}, [&temp_dir]() { fs::remove_all(temp_dir); });

    write_file(temp_dir / "assets.json", R"({"ns": "test"})");

    SECTION("Parse single label with custom properties") {
        write_file(temp_dir / "ui" / "label_custom.json", R"({
            "title_label": {
                "type": "label",
                "text": "Serenkai Title",
                "anchor": "Center",
                "offset": [20, -10],
                "color": "Yellow"
            }
        })");

        TestParserContext ctx(temp_dir);
        auto view = ctx.parser.parse("test:ui/label_custom.json");
        REQUIRE(view != nullptr);
        CHECK(view->loc() ==
              *ResourceLocation::parse("test:ui/label_custom.json"));

        auto* widget = view->widget();
        REQUIRE(widget != nullptr);
        CHECK(widget->name() == "title_label");
        CHECK(widget->anchor() == Anchor::Center);
        CHECK(widget->offset() == glm::ivec2{20, -10});

        auto* label = dynamic_cast<Label*>(widget);
        REQUIRE(label != nullptr);
        CHECK(label->text() == "Serenkai Title");
        CHECK(label->color() == Color::Yellow);
    }

    SECTION("Parse label with default properties") {
        write_file(temp_dir / "ui" / "label_default.json", R"({
            "default_label": {
                "type": "label"
            }
        })");

        TestParserContext ctx(temp_dir);
        auto view = ctx.parser.parse("test:ui/label_default.json");
        REQUIRE(view != nullptr);

        auto* widget = view->widget();
        REQUIRE(widget != nullptr);
        CHECK(widget->name() == "default_label");
        CHECK(widget->anchor() == Anchor::TopLeft);
        CHECK(widget->offset() == glm::ivec2{0, 0});

        auto* label = dynamic_cast<Label*>(widget);
        REQUIRE(label != nullptr);
        CHECK(label->text().empty());
        CHECK(label->color() == Color::White);
    }
}

TEST_CASE("WidgetParser rect construction and property parsing",
          "[gui][parser]") {
    fs::path temp_dir = fs::temp_directory_path() / "serenkai_test_parser_rect";
    fs::remove_all(temp_dir);
    fs::create_directories(temp_dir / "ui");
    RaiiGuard cleanup([]() {}, [&temp_dir]() { fs::remove_all(temp_dir); });

    write_file(temp_dir / "assets.json", R"({"ns": "test"})");

    SECTION("Parse single rect with explicit size and custom properties") {
        write_file(temp_dir / "ui" / "rect_custom.json", R"({
            "panel_rect": {
                "type": "rect",
                "size": [320, 240],
                "color": "Blue",
                "alpha": 0.8,
                "anchor": "Center",
                "offset": [10, -5]
            }
        })");

        TestParserContext ctx(temp_dir);
        auto view = ctx.parser.parse("test:ui/rect_custom.json");
        REQUIRE(view != nullptr);

        auto* widget = view->widget();
        REQUIRE(widget != nullptr);
        CHECK(widget->name() == "panel_rect");
        CHECK(widget->anchor() == Anchor::Center);
        CHECK(widget->offset() == glm::ivec2{10, -5});

        auto* rect = dynamic_cast<Rect*>(widget);
        REQUIRE(rect != nullptr);
        CHECK(rect->size() == glm::ivec2{320, 240});
        CHECK(rect->color() == Color::Blue);
        CHECK(rect->alpha() == 0.8f);
        CHECK_FALSE(rect->fill_parent());
    }

    SECTION("Parse rect with fill_parent") {
        write_file(temp_dir / "ui" / "rect_fill.json", R"({
            "background_rect": {
                "type": "rect",
                "fill_parent": true,
                "color": "Black"
            }
        })");

        TestParserContext ctx(temp_dir);
        auto view = ctx.parser.parse("test:ui/rect_fill.json");
        REQUIRE(view != nullptr);

        auto* widget = view->widget();
        REQUIRE(widget != nullptr);
        CHECK(widget->name() == "background_rect");

        auto* rect = dynamic_cast<Rect*>(widget);
        REQUIRE(rect != nullptr);
        CHECK(rect->fill_parent());
        CHECK(rect->color() == Color::Black);
        CHECK(rect->alpha() == 1.0f);
    }

    SECTION("Parse rect with children") {
        write_file(temp_dir / "ui" / "rect_tree.json", R"({
            "parent_rect": {
                "type": "rect",
                "size": [400, 300],
                "children": {
                    "child_rect": {
                        "type": "rect",
                        "fill_parent": true
                    }
                }
            }
        })");

        TestParserContext ctx(temp_dir);
        auto view = ctx.parser.parse("test:ui/rect_tree.json");
        REQUIRE(view != nullptr);

        auto* root = view->widget();
        REQUIRE(root != nullptr);
        CHECK(root->name() == "parent_rect");
        CHECK(root->children().size() == 1);

        auto* child = root->fetch_child<Rect>("child_rect");
        REQUIRE(child != nullptr);
        CHECK(child->fill_parent());
        CHECK(child->parent() == root);
    }
}

TEST_CASE("WidgetParser hierarchical widget tree construction",
          "[gui][parser]") {
    fs::path temp_dir = fs::temp_directory_path() / "serenkai_test_parser_tree";
    fs::remove_all(temp_dir);
    fs::create_directories(temp_dir / "ui");
    RaiiGuard cleanup([]() {}, [&temp_dir]() { fs::remove_all(temp_dir); });

    write_file(temp_dir / "assets.json", R"({"ns": "test"})");
    write_file(temp_dir / "ui" / "tree.json", R"({
        "parent_label": {
            "type": "label",
            "text": "Parent",
            "children": {
                "child_a": {
                    "type": "label",
                    "text": "Child A",
                    "anchor": "BottomRight"
                },
                "child_b": {
                    "type": "label",
                    "text": "Child B",
                    "color": "Green"
                }
            }
        }
    })");

    TestParserContext ctx(temp_dir);
    auto view = ctx.parser.parse("test:ui/tree.json");
    REQUIRE(view != nullptr);

    auto* root = view->widget();
    REQUIRE(root != nullptr);
    CHECK(root->name() == "parent_label");
    CHECK(root->parent() == nullptr);
    CHECK(root->children().size() == 2);

    auto* child_a = root->fetch_child<Label>("child_a");
    REQUIRE(child_a != nullptr);
    CHECK(child_a->text() == "Child A");
    CHECK(child_a->anchor() == Anchor::BottomRight);
    CHECK(child_a->parent() == root);

    auto* child_b = root->fetch_child<Label>("child_b");
    REQUIRE(child_b != nullptr);
    CHECK(child_b->text() == "Child B");
    CHECK(child_b->color() == Color::Green);
    CHECK(child_b->parent() == root);
}

TEST_CASE("WidgetParser error handling and boundary conditions",
          "[gui][parser]") {
    fs::path temp_dir = fs::temp_directory_path() / "serenkai_test_parser_err";
    fs::remove_all(temp_dir);
    fs::create_directories(temp_dir / "ui");
    RaiiGuard cleanup([]() {}, [&temp_dir]() { fs::remove_all(temp_dir); });

    write_file(temp_dir / "assets.json", R"({"ns": "test"})");

    SECTION("Non-existent resource location returns nullptr") {
        TestParserContext ctx(temp_dir);
        auto view = ctx.parser.parse("test:ui/missing.json");
        CHECK(view == nullptr);
    }

    SECTION("Invalid JSON syntax returns nullptr") {
        write_file(temp_dir / "ui" / "syntax_err.json", "{ invalid json }");
        TestParserContext ctx(temp_dir);
        auto view = ctx.parser.parse("test:ui/syntax_err.json");
        CHECK(view == nullptr);
    }

    SECTION("Non-object JSON root returns nullptr") {
        write_file(temp_dir / "ui" / "array_root.json", R"([
            { "title": { "type": "label" } }
        ])");
        TestParserContext ctx(temp_dir);
        auto view = ctx.parser.parse("test:ui/array_root.json");
        CHECK(view == nullptr);
    }

    SECTION("Multiple root widgets return nullptr") {
        write_file(temp_dir / "ui" / "multi_root.json", R"({
            "widget1": { "type": "label" },
            "widget2": { "type": "label" }
        })");
        TestParserContext ctx(temp_dir);
        auto view = ctx.parser.parse("test:ui/multi_root.json");
        CHECK(view == nullptr);
    }

    SECTION("Missing widget type field returns nullptr") {
        write_file(temp_dir / "ui" / "no_type.json", R"({
            "widget1": {
                "text": "No Type"
            }
        })");
        TestParserContext ctx(temp_dir);
        auto view = ctx.parser.parse("test:ui/no_type.json");
        CHECK(view == nullptr);
    }

    SECTION("Unknown widget type returns nullptr") {
        write_file(temp_dir / "ui" / "unknown_type.json", R"({
            "widget1": {
                "type": "unregistered_widget_type"
            }
        })");
        TestParserContext ctx(temp_dir);
        auto view = ctx.parser.parse("test:ui/unknown_type.json");
        CHECK(view == nullptr);
    }

    SECTION("Children field is not an object gracefully retains parent") {
        write_file(temp_dir / "ui" / "invalid_children.json", R"({
            "widget1": {
                "type": "label",
                "text": "Parent Only",
                "children": "not an object"
            }
        })");
        TestParserContext ctx(temp_dir);
        auto view = ctx.parser.parse("test:ui/invalid_children.json");
        REQUIRE(view != nullptr);

        auto* widget = view->widget();
        REQUIRE(widget != nullptr);
        CHECK(widget->children().empty());
    }

    SECTION("Rect missing both size and fill_parent returns nullptr") {
        write_file(temp_dir / "ui" / "rect_no_size.json", R"({
            "invalid_rect": {
                "type": "rect",
                "color": "Red"
            }
        })");
        TestParserContext ctx(temp_dir);
        auto view = ctx.parser.parse("test:ui/rect_no_size.json");
        CHECK(view == nullptr);
    }
}

TEST_CASE("WidgetParser button construction, property parsing, and callbacks",
          "[gui][parser]") {
    fs::path temp_dir = fs::temp_directory_path() / "serenkai_test_parser_btn";
    fs::remove_all(temp_dir);
    fs::create_directories(temp_dir / "ui");
    RaiiGuard cleanup([]() {}, [&temp_dir]() { fs::remove_all(temp_dir); });

    write_file(temp_dir / "assets.json", R"({"ns": "test"})");

    SECTION("Parse button with anchor, offset, children, and callback") {
        write_file(temp_dir / "ui" / "button.json", R"({
            "submit_button": {
                "type": "button",
                "anchor": "Center",
                "offset": [10, -5],
                "callback": "on_submit",
                "children": {
                    "btn_bg": {
                        "type": "rect",
                        "size": [100, 40]
                    }
                }
            }
        })");

        TestParserContext ctx(temp_dir);

        auto view = ctx.parser.parse("test:ui/button.json");
        REQUIRE(view != nullptr);

        auto* widget = view->widget();
        REQUIRE(widget != nullptr);
        CHECK(widget->name() == "submit_button");
        CHECK(widget->anchor() == Anchor::Center);
        CHECK(widget->offset() == glm::ivec2{10, -5});

        auto* button = dynamic_cast<Button*>(widget);
        REQUIRE(button != nullptr);
        CHECK(button->is_enabled());
        CHECK_FALSE(button->is_hovered());
        CHECK(button->callback_name() == "on_submit");

        // Update to measure children
        button->update(0.016f);
        CHECK(button->size() == glm::ivec2{100, 40});

        auto* child_bg = button->fetch_child<Rect>("btn_bg");
        REQUIRE(child_bg != nullptr);
    }

    SECTION("Button without callback still parses successfully") {
        write_file(temp_dir / "ui" / "btn_no_cb.json", R"({
            "plain_btn": {
                "type": "button"
            }
        })");

        TestParserContext ctx(temp_dir);
        auto view = ctx.parser.parse("test:ui/btn_no_cb.json");
        REQUIRE(view != nullptr);

        auto* widget = view->widget();
        REQUIRE(widget != nullptr);
        auto* button = dynamic_cast<Button*>(widget);
        REQUIRE(button != nullptr);
        CHECK(button->name() == "plain_btn");
    }
}

TEST_CASE("WidgetParser image construction and property parsing",
          "[gui][parser]") {
    fs::path temp_dir = fs::temp_directory_path() / "serenkai_test_parser_img";
    fs::remove_all(temp_dir);
    fs::create_directories(temp_dir / "ui");
    RaiiGuard cleanup([]() {}, [&temp_dir]() { fs::remove_all(temp_dir); });

    write_file(temp_dir / "assets.json", R"({"ns": "test"})");

    SECTION("Parse image with explicit size and custom anchor/offset") {
        write_file(temp_dir / "ui" / "image_explicit.json", R"({
            "avatar": {
                "type": "image",
                "image": "test:textures/avatar.png",
                "size": [64, 48],
                "anchor": "Center",
                "offset": [15, -10]
            }
        })");

        TestParserContext ctx(temp_dir);
        auto view = ctx.parser.parse("test:ui/image_explicit.json");
        REQUIRE(view != nullptr);

        auto* widget = view->widget();
        REQUIRE(widget != nullptr);
        CHECK(widget->name() == "avatar");
        CHECK(widget->anchor() == Anchor::Center);
        CHECK(widget->offset() == glm::ivec2{15, -10});

        auto* img = dynamic_cast<ImageWidget*>(widget);
        REQUIRE(img != nullptr);
        CHECK(img->get_image() == "test:textures/avatar.png");
        CHECK(img->size() == glm::ivec2{64, 48});
    }

    SECTION("Parse image with children") {
        write_file(temp_dir / "ui" / "image_tree.json", R"({
            "banner": {
                "type": "image",
                "image": "test:textures/banner.png",
                "size": [300, 100],
                "children": {
                    "title": {
                        "type": "label",
                        "text": "Header"
                    }
                }
            }
        })");

        TestParserContext ctx(temp_dir);
        auto view = ctx.parser.parse("test:ui/image_tree.json");
        REQUIRE(view != nullptr);

        auto* widget = view->widget();
        REQUIRE(widget != nullptr);
        CHECK(widget->children().size() == 1);
        auto* child = widget->fetch_child<Label>("title");
        REQUIRE(child != nullptr);
        CHECK(child->text() == "Header");
    }

    SECTION("Image missing required image field returns nullptr") {
        write_file(temp_dir / "ui" / "image_no_src.json", R"({
            "bad_image": {
                "type": "image",
                "size": [32, 32]
            }
        })");

        TestParserContext ctx(temp_dir);
        auto view = ctx.parser.parse("test:ui/image_no_src.json");
        CHECK(view == nullptr);
    }

    SECTION("Parse image with auto size measurement") {
        SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "dummy");
        REQUIRE(SDL_Init(SDL_INIT_VIDEO));

        SDL_Window* window = SDL_CreateWindow("Test", 64, 64, 0);
        REQUIRE(window != nullptr);

        SDL_Renderer* renderer = SDL_CreateRenderer(window, nullptr);
        REQUIRE(renderer != nullptr);

        RaiiGuard sdl_guard([]() {},
                            [&]() {
                                SDL_DestroyRenderer(renderer);
                                SDL_DestroyWindow(window);
                                SDL_Quit();
                            });

        const int width = 12;
        const int height = 8;
        const int channels = 4;
        const std::vector<uint8_t> pixels(width * height * channels, 255);
        fs::path img_path = temp_dir / "icon12x8.png";
        int write_res =
            stbi_write_png(img_path.string().c_str(), width, height, channels,
                           pixels.data(), width * channels);
        REQUIRE(write_res != 0);

        write_file(temp_dir / "ui" / "image_auto_size.json", R"({
            "measured_image": {
                "type": "image",
                "image": "test:icon12x8.png"
            }
        })");

        TestParserContext ctx(temp_dir, renderer);
        auto view = ctx.parser.parse("test:ui/image_auto_size.json");
        REQUIRE(view != nullptr);

        auto* widget = view->widget();
        REQUIRE(widget != nullptr);
        CHECK(widget->name() == "measured_image");

        auto* img = dynamic_cast<ImageWidget*>(widget);
        REQUIRE(img != nullptr);
        CHECK(img->get_image() == "test:icon12x8.png");
        CHECK(img->size() == glm::ivec2{12, 8});
    }
}

TEST_CASE("WidgetParser column construction and property parsing",
          "[gui][parser]") {
    fs::path temp_dir = fs::temp_directory_path() / "serenkai_test_parser_col";
    fs::remove_all(temp_dir);
    fs::create_directories(temp_dir / "ui");
    RaiiGuard cleanup([]() {}, [&temp_dir]() { fs::remove_all(temp_dir); });

    write_file(temp_dir / "assets.json", R"({"ns": "test"})");

    SECTION("Parse column with default properties") {
        write_file(temp_dir / "ui" / "col_default.json", R"({
            "menu_col": {
                "type": "column"
            }
        })");

        TestParserContext ctx(temp_dir);
        auto view = ctx.parser.parse("test:ui/col_default.json");
        REQUIRE(view != nullptr);

        auto* widget = view->widget();
        REQUIRE(widget != nullptr);
        CHECK(widget->name() == "menu_col");
        CHECK(widget->anchor() == Anchor::TopLeft);
        CHECK(widget->offset() == glm::ivec2{0, 0});

        auto* col = dynamic_cast<ColumnLayout*>(widget);
        REQUIRE(col != nullptr);
        CHECK(col->spacing() == 0);
        CHECK(col->child_anchor() == ChildAnchor::Left);
        CHECK(col->size() == glm::ivec2{0, 0});
    }

    SECTION("Parse column with custom properties") {
        write_file(temp_dir / "ui" / "col_custom.json", R"({
            "nav_col": {
                "type": "column",
                "spacing": 15,
                "child_anchor": "Center",
                "anchor": "Center",
                "offset": [10, -20]
            }
        })");

        TestParserContext ctx(temp_dir);
        auto view = ctx.parser.parse("test:ui/col_custom.json");
        REQUIRE(view != nullptr);

        auto* widget = view->widget();
        REQUIRE(widget != nullptr);
        CHECK(widget->name() == "nav_col");
        CHECK(widget->anchor() == Anchor::Center);
        CHECK(widget->offset() == glm::ivec2{10, -20});

        auto* col = dynamic_cast<ColumnLayout*>(widget);
        REQUIRE(col != nullptr);
        CHECK(col->spacing() == 15);
        CHECK(col->child_anchor() == ChildAnchor::Center);
    }

    SECTION("Parse column with children and verify layout") {
        write_file(temp_dir / "ui" / "col_tree.json", R"({
            "box_col": {
                "type": "column",
                "spacing": 10,
                "child_anchor": "Right",
                "children": {
                    "item1": {
                        "type": "rect",
                        "size": [100, 40]
                    },
                    "item2": {
                        "type": "rect",
                        "size": [50, 30]
                    }
                }
            }
        })");

        TestParserContext ctx(temp_dir);
        auto view = ctx.parser.parse("test:ui/col_tree.json");
        REQUIRE(view != nullptr);

        auto* widget = view->widget();
        REQUIRE(widget != nullptr);

        auto* col = dynamic_cast<ColumnLayout*>(widget);
        REQUIRE(col != nullptr);
        CHECK(col->children().size() == 2);
        CHECK(col->size() == glm::ivec2{100, 80});

        auto* item1 = col->fetch_child<Rect>("item1");
        REQUIRE(item1 != nullptr);
        CHECK(item1->anchor() == Anchor::TopRight);
        CHECK(item1->offset() == glm::ivec2{0, 0});

        auto* item2 = col->fetch_child<Rect>("item2");
        REQUIRE(item2 != nullptr);
        CHECK(item2->anchor() == Anchor::TopRight);
        CHECK(item2->offset() == glm::ivec2{0, 50});
    }
}

TEST_CASE("WidgetParser script parsing and UiView bundling", "[gui][parser]") {
    fs::path temp_dir =
        fs::temp_directory_path() / "serenkai_test_parser_script";
    fs::remove_all(temp_dir);
    fs::create_directories(temp_dir / "ui");
    fs::create_directories(temp_dir / "scripts");
    RaiiGuard cleanup([]() {}, [&temp_dir]() { fs::remove_all(temp_dir); });

    write_file(temp_dir / "assets.json", R"({"ns": "test"})");
    write_file(temp_dir / "scripts" / "view_test.luau", R"(
        function get_value()
            return 42
        end
    )");

    SECTION("Parse UI with valid script attached") {
        write_file(temp_dir / "ui" / "view_with_script.json", R"({
            "script": "test:scripts/view_test.luau",
            "root_panel": {
                "type": "rect",
                "fill_parent": true
            }
        })");

        TestParserContext ctx(temp_dir);
        auto view = ctx.parser.parse("test:ui/view_with_script.json");
        REQUIRE(view != nullptr);
        CHECK(view->loc() ==
              *ResourceLocation::parse("test:ui/view_with_script.json"));
        REQUIRE(view->widget() != nullptr);
        CHECK(view->widget()->name() == "root_panel");
        REQUIRE(view->script() != nullptr);

        auto res = view->script()->call<int>("get_value");
        REQUIRE(res.has_value());
        CHECK(std::get<0>(*res) == 42);
    }

    SECTION("Parse UI with non-existent script returns nullptr") {
        write_file(temp_dir / "ui" / "view_missing_script.json", R"({
            "script": "test:scripts/does_not_exist.luau",
            "root_panel": {
                "type": "rect",
                "fill_parent": true
            }
        })");

        TestParserContext ctx(temp_dir);
        auto view = ctx.parser.parse("test:ui/view_missing_script.json");
        CHECK(view == nullptr);
    }

    SECTION("Parse UI with non-string script field returns nullptr") {
        write_file(temp_dir / "ui" / "view_invalid_script_type.json", R"({
            "script": 12345,
            "root_panel": {
                "type": "rect",
                "fill_parent": true
            }
        })");

        TestParserContext ctx(temp_dir);
        auto view = ctx.parser.parse("test:ui/view_invalid_script_type.json");
        CHECK(view == nullptr);
    }

    SECTION("Parse UI with script when ScriptEngine is nullptr skips script "
            "gracefully") {
        write_file(temp_dir / "ui" / "view_no_engine.json", R"({
            "script": "test:scripts/view_test.luau",
            "root_panel": {
                "type": "rect",
                "fill_parent": true
            }
        })");

        AssetManager asset_manager;
        FontManager font_manager(&asset_manager);
        TextureManager texture_manager(&asset_manager, nullptr);
        asset_manager.merge_source(std::make_shared<DirectorySource>(temp_dir));

        WidgetParser parser(WidgetParserConfig{
            &asset_manager, &font_manager, &texture_manager, nullptr, nullptr});

        auto view = parser.parse("test:ui/view_no_engine.json");
        REQUIRE(view != nullptr);
        CHECK(view->script() == nullptr);
        CHECK(view->widget() != nullptr);
    }
}

TEST_CASE("UiView event and update delegation", "[gui][view]") {
    auto loc = *ResourceLocation::parse("test:ui/delegation_view.json");

    SECTION("UiView forwards update and events to root widget") {
        auto button = std::make_unique<Button>("test_btn", nullptr);
        auto bg = std::make_unique<Rect>("btn_bg", button.get());
        bg->set_size(glm::ivec2{80, 30});
        button->add_child(std::move(bg));

        int click_count = 0;
        button->set_clicked([&click_count]() { ++click_count; });

        UiView view(loc, std::move(button), nullptr);
        CHECK(view.loc() == loc);
        REQUIRE(view.widget() != nullptr);
        CHECK(view.script() == nullptr);

        // Update call forwards and measures button size
        view.update(0.016f);
        CHECK(view.widget()->size() == glm::ivec2{80, 30});

        // Hover test via handle_event
        MouseMoveEvent move_in{10.0f, 10.0f, 0.0f, 0.0f};
        view.handle_event(move_in);
        auto* btn = dynamic_cast<Button*>(view.widget());
        REQUIRE(btn != nullptr);
        CHECK(btn->is_hovered());

        // Click event forwards
        KeyEvent left_click{Key::MouseLeft, KeyAction::Press};
        CHECK(view.handle_event(left_click));
        CHECK(click_count == 1);

        // Other events return false
        WindowResizeEvent resize_event{800, 600};
        CHECK_FALSE(view.handle_event(resize_event));
    }

    SECTION("UiView handles null widget safely") {
        UiView view(loc, nullptr, nullptr);
        view.update(0.016f);
        view.render(nullptr);

        KeyEvent key_event{Key::MouseLeft, KeyAction::Press};
        CHECK_FALSE(view.handle_event(key_event));
    }
}
