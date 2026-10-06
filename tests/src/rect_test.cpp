#include "serenkai/gui/color.hpp"
#include "serenkai/gui/rect.hpp"
#include "serenkai/gui/widget.hpp"

#include <catch2/catch_test_macros.hpp>

using namespace serenkai;

TEST_CASE("Rect default state and property setters", "[gui][rect]") {
    Rect rect("rect", nullptr);

    CHECK(rect.name() == "rect");
    CHECK(rect.color() == Color::White);
    CHECK(rect.alpha() == 1.0f);
    CHECK_FALSE(rect.fill_parent());
    CHECK(rect.size() == glm::ivec2{0, 0});
    CHECK(rect.parent() == nullptr);
    CHECK_FALSE(rect.has_parent());

    SECTION("Color mutation") {
        rect.set_color(Color::Red);
        CHECK(rect.color() == Color::Red);
    }

    SECTION("Alpha mutation") {
        rect.set_alpha(0.5f);
        CHECK(rect.alpha() == 0.5f);
    }

    SECTION("Explicit size mutation") {
        rect.set_size({120, 80});
        CHECK(rect.size() == glm::ivec2{120, 80});
    }

    SECTION("Fill parent property mutation") {
        rect.set_fill_parent(true);
        CHECK(rect.fill_parent());
    }
}

TEST_CASE("Rect fill_parent behavior", "[gui][rect]") {
    SECTION("Root rect fills logical window size upon update") {
        Widget::set_logical_window_size({800, 600});

        Rect rect("root_rect", nullptr);
        rect.set_fill_parent(true);
        CHECK(rect.fill_parent());
        CHECK(rect.size() == glm::ivec2{0, 0});

        rect.update(0.016f);
        CHECK(rect.size() == glm::ivec2{800, 600});

        // set_size should have no effect when fill_parent is true
        rect.set_size({100, 100});
        CHECK(rect.size() == glm::ivec2{800, 600});

        // Updating logical window size updates rect size upon update
        Widget::set_logical_window_size({1024, 768});
        rect.update(0.016f);
        CHECK(rect.size() == glm::ivec2{1024, 768});
    }

    SECTION("Child rect fills parent size upon update") {
        Widget parent("parent", nullptr);
        parent.set_size({400, 300});

        auto& child_rect = parent.create_child<Rect>("child_rect");
        CHECK(child_rect.has_parent());
        CHECK(child_rect.parent() == &parent);

        child_rect.set_fill_parent(true);
        parent.update(0.016f);
        CHECK(child_rect.size() == glm::ivec2{400, 300});

        // Updating parent size updates child rect size upon update
        parent.set_size({500, 450});
        parent.update(0.016f);
        CHECK(child_rect.size() == glm::ivec2{500, 450});
    }
}

TEST_CASE("Rect render safety", "[gui][rect]") {
    Rect rect("rect", nullptr);

    SECTION("Render safety without context") { rect.render(nullptr); }

    SECTION("Invisible rect skips render without error") {
        rect.set_visible(false);
        rect.render(nullptr);
    }
}
