#include "serenkai/gui/anchor.hpp"
#include "serenkai/gui/column_layout.hpp"
#include "serenkai/gui/rect.hpp"
#include "serenkai/gui/widget.hpp"

#include <catch2/catch_test_macros.hpp>

using namespace serenkai;

TEST_CASE("ColumnLayout default state and property setters",
          "[gui][column_layout]") {
    ColumnLayout column("test_col", nullptr);

    CHECK(column.name() == "test_col");
    CHECK(column.parent() == nullptr);
    CHECK(column.children().empty());
    CHECK(column.size() == glm::ivec2{0, 0});
    CHECK(column.spacing() == 0);
    CHECK(column.child_anchor() == ChildAnchor::Left);

    SECTION("Spacing mutation") {
        column.set_spacing(12);
        CHECK(column.spacing() == 12);
    }

    SECTION("Child anchor mutation") {
        column.set_child_anchor(ChildAnchor::Center);
        CHECK(column.child_anchor() == ChildAnchor::Center);

        column.set_child_anchor(ChildAnchor::Right);
        CHECK(column.child_anchor() == ChildAnchor::Right);
    }
}

TEST_CASE("ColumnLayout size calculation and layout with children",
          "[gui][column_layout]") {
    Widget::set_logical_window_size({1280, 720});
    ColumnLayout column("col", nullptr);

    SECTION("Empty column layout has zero size") {
        column.layout();
        CHECK(column.size() == glm::ivec2{0, 0});
    }

    SECTION("Single child size matches child") {
        auto& r1 = column.create_child<Rect>("r1");
        r1.set_size({120, 40});

        column.layout();
        CHECK(column.size() == glm::ivec2{120, 40});
        CHECK(r1.offset() == glm::ivec2{0, 0});
        CHECK(r1.pos() == glm::ivec2{0, 0});
    }

    SECTION("Multiple children stack vertically with spacing") {
        column.set_spacing(10);

        auto& r1 = column.create_child<Rect>("r1");
        r1.set_size({100, 30});

        auto& r2 = column.create_child<Rect>("r2");
        r2.set_size({60, 50});

        auto& r3 = column.create_child<Rect>("r3");
        r3.set_size({80, 20});

        column.layout();

        // Total width = max(100, 60, 80) = 100
        // Total height = 30 + 10 + 50 + 10 + 20 = 120
        CHECK(column.size() == glm::ivec2{100, 120});

        // Vertical offsets
        CHECK(r1.offset() == glm::ivec2{0, 0});
        CHECK(r2.offset() == glm::ivec2{0, 40});
        CHECK(r3.offset() == glm::ivec2{0, 100});
    }

    SECTION("Horizontal alignment via ChildAnchor") {
        auto& r1 = column.create_child<Rect>("r1");
        r1.set_size({100, 30});

        auto& r2 = column.create_child<Rect>("r2");
        r2.set_size({60, 40});

        SECTION("Left alignment") {
            column.set_child_anchor(ChildAnchor::Left);
            column.layout();

            CHECK(r1.anchor() == Anchor::TopLeft);
            CHECK(r2.anchor() == Anchor::TopLeft);
            CHECK(r1.pos() == glm::ivec2{0, 0});
            CHECK(r2.pos() == glm::ivec2{0, 30});
        }

        SECTION("Center alignment") {
            column.set_child_anchor(ChildAnchor::Center);
            column.layout();

            CHECK(r1.anchor() == Anchor::TopCenter);
            CHECK(r2.anchor() == Anchor::TopCenter);
            // Column width is 100.
            // r1: (100 - 100) / 2 = 0
            // r2: (100 - 60) / 2 = 20
            CHECK(r1.pos() == glm::ivec2{0, 0});
            CHECK(r2.pos() == glm::ivec2{20, 30});
        }

        SECTION("Right alignment") {
            column.set_child_anchor(ChildAnchor::Right);
            column.layout();

            CHECK(r1.anchor() == Anchor::TopRight);
            CHECK(r2.anchor() == Anchor::TopRight);
            // Column width is 100.
            // r1: 100 - 100 = 0
            // r2: 100 - 60 = 40
            CHECK(r1.pos() == glm::ivec2{0, 0});
            CHECK(r2.pos() == glm::ivec2{40, 30});
        }
    }

    SECTION("Update lifecycle automatically invokes layout") {
        column.set_spacing(5);
        auto& r1 = column.create_child<Rect>("r1");
        r1.set_size({50, 20});
        auto& r2 = column.create_child<Rect>("r2");
        r2.set_size({70, 30});

        CHECK(column.size() == glm::ivec2{0, 0});

        column.update(0.016f);

        CHECK(column.size() == glm::ivec2{70, 55});
        CHECK(r1.offset() == glm::ivec2{0, 0});
        CHECK(r2.offset() == glm::ivec2{0, 25});
    }
}
