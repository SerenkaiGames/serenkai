#include "serenkai/base/concepts.hpp"
#include "serenkai/base/raii.hpp"
#include "serenkai/base/type_name.hpp"
#include "serenkai/gui/anchor.hpp"
#include "serenkai/gui/color.hpp"
#include "serenkai/gui/label.hpp"
#include "serenkai/gui/rect.hpp"
#include "serenkai/gui/widget_parser.hpp"
#include "serenkai/resource/asset_manager.hpp"
#include "serenkai/resource/directory_source.hpp"
#include "serenkai/resource/font_manager.hpp"

#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include <fstream>
#include <memory>
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
    WidgetParser parser;

    explicit TestParserContext(const fs::path& dir)
        : font_manager(&asset_manager), parser(&asset_manager, &font_manager) {
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
            "root": {
                "title_label": {
                    "type": "label",
                    "text": "Serenkai Title",
                    "anchor": "Center",
                    "offset": [20, -10],
                    "color": "Yellow"
                }
            }
        })");

        TestParserContext ctx(temp_dir);
        auto widget = ctx.parser.parse("test:ui/label_custom.json");
        REQUIRE(widget != nullptr);
        CHECK(widget->name() == "title_label");
        CHECK(widget->anchor() == Anchor::Center);
        CHECK(widget->offset() == glm::ivec2{20, -10});

        auto* label = dynamic_cast<Label*>(widget.get());
        REQUIRE(label != nullptr);
        CHECK(label->text() == "Serenkai Title");
        CHECK(label->color() == Color::Yellow);
    }

    SECTION("Parse label with default properties") {
        write_file(temp_dir / "ui" / "label_default.json", R"({
            "root": {
                "default_label": {
                    "type": "label"
                }
            }
        })");

        TestParserContext ctx(temp_dir);
        auto widget = ctx.parser.parse("test:ui/label_default.json");
        REQUIRE(widget != nullptr);
        CHECK(widget->name() == "default_label");
        CHECK(widget->anchor() == Anchor::TopLeft);
        CHECK(widget->offset() == glm::ivec2{0, 0});

        auto* label = dynamic_cast<Label*>(widget.get());
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
            "root": {
                "panel_rect": {
                    "type": "rect",
                    "size": [320, 240],
                    "color": "Blue",
                    "alpha": 0.8,
                    "anchor": "Center",
                    "offset": [10, -5]
                }
            }
        })");

        TestParserContext ctx(temp_dir);
        auto widget = ctx.parser.parse("test:ui/rect_custom.json");
        REQUIRE(widget != nullptr);
        CHECK(widget->name() == "panel_rect");
        CHECK(widget->anchor() == Anchor::Center);
        CHECK(widget->offset() == glm::ivec2{10, -5});

        auto* rect = dynamic_cast<Rect*>(widget.get());
        REQUIRE(rect != nullptr);
        CHECK(rect->size() == glm::ivec2{320, 240});
        CHECK(rect->color() == Color::Blue);
        CHECK(rect->alpha() == 0.8f);
        CHECK_FALSE(rect->fill_parent());
    }

    SECTION("Parse rect with fill_parent") {
        write_file(temp_dir / "ui" / "rect_fill.json", R"({
            "root": {
                "background_rect": {
                    "type": "rect",
                    "fill_parent": true,
                    "color": "Black"
                }
            }
        })");

        TestParserContext ctx(temp_dir);
        auto widget = ctx.parser.parse("test:ui/rect_fill.json");
        REQUIRE(widget != nullptr);
        CHECK(widget->name() == "background_rect");

        auto* rect = dynamic_cast<Rect*>(widget.get());
        REQUIRE(rect != nullptr);
        CHECK(rect->fill_parent());
        CHECK(rect->color() == Color::Black);
        CHECK(rect->alpha() == 1.0f);
    }

    SECTION("Parse rect with children") {
        write_file(temp_dir / "ui" / "rect_tree.json", R"({
            "root": {
                "parent_rect": {
                    "type": "rect",
                    "size": [400, 300],
                    "children": [
                        {
                            "child_rect": {
                                "type": "rect",
                                "fill_parent": true
                            }
                        }
                    ]
                }
            }
        })");

        TestParserContext ctx(temp_dir);
        auto root = ctx.parser.parse("test:ui/rect_tree.json");
        REQUIRE(root != nullptr);
        CHECK(root->name() == "parent_rect");
        CHECK(root->children().size() == 1);

        auto* child = root->fetch_child<Rect>("child_rect");
        REQUIRE(child != nullptr);
        CHECK(child->fill_parent());
        CHECK(child->parent() == root.get());
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
        "root": {
            "parent_label": {
                "type": "label",
                "text": "Parent",
                "children": [
                    {
                        "child_a": {
                            "type": "label",
                            "text": "Child A",
                            "anchor": "BottomRight"
                        }
                    },
                    {
                        "child_b": {
                            "type": "label",
                            "text": "Child B",
                            "color": "Green"
                        }
                    }
                ]
            }
        }
    })");

    TestParserContext ctx(temp_dir);
    auto root = ctx.parser.parse("test:ui/tree.json");
    REQUIRE(root != nullptr);
    CHECK(root->name() == "parent_label");
    CHECK(root->parent() == nullptr);
    CHECK(root->children().size() == 2);

    auto* child_a = root->fetch_child<Label>("child_a");
    REQUIRE(child_a != nullptr);
    CHECK(child_a->text() == "Child A");
    CHECK(child_a->anchor() == Anchor::BottomRight);
    CHECK(child_a->parent() == root.get());

    auto* child_b = root->fetch_child<Label>("child_b");
    REQUIRE(child_b != nullptr);
    CHECK(child_b->text() == "Child B");
    CHECK(child_b->color() == Color::Green);
    CHECK(child_b->parent() == root.get());
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
        auto widget = ctx.parser.parse("test:ui/missing.json");
        CHECK(widget == nullptr);
    }

    SECTION("Invalid JSON syntax returns nullptr") {
        write_file(temp_dir / "ui" / "syntax_err.json", "{ invalid json }");
        TestParserContext ctx(temp_dir);
        auto widget = ctx.parser.parse("test:ui/syntax_err.json");
        CHECK(widget == nullptr);
    }

    SECTION("Missing root field returns nullptr") {
        write_file(temp_dir / "ui" / "no_root.json", R"({
            "not_root": {
                "title": { "type": "label" }
            }
        })");
        TestParserContext ctx(temp_dir);
        auto widget = ctx.parser.parse("test:ui/no_root.json");
        CHECK(widget == nullptr);
    }

    SECTION("Multiple root widgets return nullptr") {
        write_file(temp_dir / "ui" / "multi_root.json", R"({
            "root": {
                "widget1": { "type": "label" },
                "widget2": { "type": "label" }
            }
        })");
        TestParserContext ctx(temp_dir);
        auto widget = ctx.parser.parse("test:ui/multi_root.json");
        CHECK(widget == nullptr);
    }

    SECTION("Missing widget type field returns nullptr") {
        write_file(temp_dir / "ui" / "no_type.json", R"({
            "root": {
                "widget1": {
                    "text": "No Type"
                }
            }
        })");
        TestParserContext ctx(temp_dir);
        auto widget = ctx.parser.parse("test:ui/no_type.json");
        CHECK(widget == nullptr);
    }

    SECTION("Unknown widget type returns nullptr") {
        write_file(temp_dir / "ui" / "unknown_type.json", R"({
            "root": {
                "widget1": {
                    "type": "unregistered_widget_type"
                }
            }
        })");
        TestParserContext ctx(temp_dir);
        auto widget = ctx.parser.parse("test:ui/unknown_type.json");
        CHECK(widget == nullptr);
    }

    SECTION("Children field is not an array gracefully retains parent") {
        write_file(temp_dir / "ui" / "invalid_children.json", R"({
            "root": {
                "widget1": {
                    "type": "label",
                    "text": "Parent Only",
                    "children": "not an array"
                }
            }
        })");
        TestParserContext ctx(temp_dir);
        auto widget = ctx.parser.parse("test:ui/invalid_children.json");
        REQUIRE(widget != nullptr);
        CHECK(widget->children().empty());
    }

    SECTION("Rect missing both size and fill_parent returns nullptr") {
        write_file(temp_dir / "ui" / "rect_no_size.json", R"({
            "root": {
                "invalid_rect": {
                    "type": "rect",
                    "color": "Red"
                }
            }
        })");
        TestParserContext ctx(temp_dir);
        auto widget = ctx.parser.parse("test:ui/rect_no_size.json");
        CHECK(widget == nullptr);
    }
}
