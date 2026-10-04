#include "serenkai/gui/anchor.hpp"
#include "serenkai/gui/widget.hpp"

#include <catch2/catch_test_macros.hpp>
#include <memory>
#include <utility>

using namespace serenkai;

namespace {

/// @brief Mock widget to verify update and render lifecycle invocations.
class MockWidget : public Widget {
public:
    explicit MockWidget(std::string name, Widget* parent = nullptr)
        : Widget(std::move(name), parent) {}

    int update_count{0};
    int render_count{0};
    float last_dt{0.0F};

protected:
    void on_update(float dt) override {
        ++update_count;
        last_dt = dt;
    }

    void on_render(GuiContext*) override { ++render_count; }
};

} // namespace

TEST_CASE("Widget default state and property setters", "[gui][widget]") {
    Widget widget("test", nullptr);

    CHECK(widget.name() == "test");
    CHECK(widget.parent() == nullptr);
    CHECK(widget.children().empty());
    CHECK(widget.size() == glm::ivec2{0, 0});
    CHECK(widget.offset() == glm::ivec2{0, 0});
    CHECK(widget.anchor() == Anchor::TopLeft);
    CHECK(widget.is_visible());

    SECTION("Property mutations") {
        widget.set_size({300, 200});
        CHECK(widget.size() == glm::ivec2{300, 200});

        widget.set_offset({15, -25});
        CHECK(widget.offset() == glm::ivec2{15, -25});

        widget.set_anchor(Anchor::BottomRight);
        CHECK(widget.anchor() == Anchor::BottomRight);

        widget.set_visible(false);
        CHECK_FALSE(widget.is_visible());
    }

    SECTION("Logical window size configuration") {
        Widget::set_logical_window_size({1920, 1080});
        CHECK(Widget::logical_window_size() == glm::ivec2{1920, 1080});
    }
}

TEST_CASE("Root widget anchor position calculations", "[gui][widget]") {
    Widget::set_logical_window_size({1000, 800});
    Widget widget("test", nullptr);
    widget.set_size({200, 100});

    SECTION("Top anchors") {
        widget.set_anchor(Anchor::TopLeft);
        CHECK(widget.pos() == glm::ivec2{0, 0});

        widget.set_anchor(Anchor::TopCenter);
        CHECK(widget.pos() == glm::ivec2{(1000 - 200) / 2, 0});

        widget.set_anchor(Anchor::TopRight);
        CHECK(widget.pos() == glm::ivec2{1000 - 200, 0});
    }

    SECTION("Center anchors") {
        widget.set_anchor(Anchor::CenterLeft);
        CHECK(widget.pos() == glm::ivec2{0, (800 - 100) / 2});

        widget.set_anchor(Anchor::Center);
        CHECK(widget.pos() == glm::ivec2{(1000 - 200) / 2, (800 - 100) / 2});

        widget.set_anchor(Anchor::CenterRight);
        CHECK(widget.pos() == glm::ivec2{1000 - 200, (800 - 100) / 2});
    }

    SECTION("Bottom anchors") {
        widget.set_anchor(Anchor::BottomLeft);
        CHECK(widget.pos() == glm::ivec2{0, 800 - 100});

        widget.set_anchor(Anchor::BottomCenter);
        CHECK(widget.pos() == glm::ivec2{(1000 - 200) / 2, 800 - 100});

        widget.set_anchor(Anchor::BottomRight);
        CHECK(widget.pos() == glm::ivec2{1000 - 200, 800 - 100});
    }

    SECTION("Anchor with offset") {
        widget.set_anchor(Anchor::Center);
        widget.set_offset({50, -30});
        CHECK(widget.pos() ==
              glm::ivec2{(1000 - 200) / 2 + 50, (800 - 100) / 2 - 30});
    }
}

TEST_CASE("Hierarchical widget positions", "[gui][widget]") {
    Widget::set_logical_window_size({1280, 720});

    Widget root("test", nullptr);
    root.set_size({400, 300});
    root.set_anchor(Anchor::TopLeft);
    root.set_offset({100, 50});
    // root.pos() should be {100, 50}
    REQUIRE(root.pos() == glm::ivec2{100, 50});

    SECTION("Child positioned relative to parent") {
        auto& child = root.create_child<Widget>("test_child");
        child.set_size({100, 60});

        child.set_anchor(Anchor::TopLeft);
        CHECK(child.pos() == glm::ivec2{100, 50});

        child.set_anchor(Anchor::Center);
        CHECK(child.pos() ==
              glm::ivec2{100 + (400 - 100) / 2, 50 + (300 - 60) / 2});

        child.set_anchor(Anchor::BottomRight);
        CHECK(child.pos() == glm::ivec2{100 + (400 - 100), 50 + (300 - 60)});

        child.set_offset({10, -5});
        CHECK(child.pos() ==
              glm::ivec2{100 + (400 - 100) + 10, 50 + (300 - 60) - 5});
    }

    SECTION("Multi-level widget nesting") {
        auto& child = root.create_child<Widget>("child");
        child.set_size({200, 200});
        child.set_anchor(Anchor::TopLeft);
        child.set_offset({20, 20});
        // child.pos() == {120, 70}

        auto& grandchild = child.create_child<Widget>("grandchild");
        grandchild.set_size({50, 50});
        grandchild.set_anchor(Anchor::BottomRight);
        grandchild.set_offset({5, 5});

        // grandchild pos = child.pos() + (200 - 50, 200 - 50) + (5, 5)
        //                = {120 + 150 + 5, 70 + 150 + 5} = {275, 225}
        CHECK(grandchild.pos() == glm::ivec2{275, 225});
    }
}

TEST_CASE("Child widget management and reparenting", "[gui][widget]") {
    Widget root("root", nullptr);
    REQUIRE(root.children().empty());

    SECTION("create_child binds parent and stores child") {
        auto& child = root.create_child<Widget>("child");
        CHECK(child.parent() == &root);
        REQUIRE(root.children().size() == 1);
        CHECK(root.children()[0].get() == &child);
    }

    SECTION("add_child transfers ownership and assigns parent") {
        auto standalone = std::make_unique<Widget>("standalone", nullptr);
        auto* raw_ptr = standalone.get();
        CHECK(standalone->parent() == nullptr);

        root.add_child(std::move(standalone));
        CHECK(raw_ptr->parent() == &root);
        REQUIRE(root.children().size() == 1);
        CHECK(root.children()[0].get() == raw_ptr);
    }

    SECTION("add_child ignores nullptr") {
        root.add_child(nullptr);
        CHECK(root.children().empty());
    }
}

TEST_CASE("Widget lifecycle update and render propagation", "[gui][widget]") {
    MockWidget root("root", nullptr);
    auto& child = root.create_child<MockWidget>("child");
    auto& grandchild = child.create_child<MockWidget>("grandchild");

    SECTION("Update propagates through hierarchy") {
        root.update(0.016F);

        CHECK(root.update_count == 1);
        CHECK(root.last_dt == 0.016F);

        CHECK(child.update_count == 1);
        CHECK(child.last_dt == 0.016F);

        CHECK(grandchild.update_count == 1);
        CHECK(grandchild.last_dt == 0.016F);
    }

    SECTION("Render propagates when visible") {
        root.render(nullptr);

        CHECK(root.render_count == 1);
        CHECK(child.render_count == 1);
        CHECK(grandchild.render_count == 1);
    }

    SECTION("Root invisible skips entire tree rendering") {
        root.set_visible(false);
        root.render(nullptr);

        CHECK(root.render_count == 0);
        CHECK(child.render_count == 0);
        CHECK(grandchild.render_count == 0);
    }

    SECTION("Child invisible skips itself and its descendants") {
        child.set_visible(false);
        root.render(nullptr);

        CHECK(root.render_count == 1);
        CHECK(child.render_count == 0);
        CHECK(grandchild.render_count == 0);
    }
}

TEST_CASE("Widget lookup by name via fetch_child", "[gui][widget]") {
    Widget root("root", nullptr);
    auto& widget_child = root.create_child<Widget>("button");
    auto& mock_child = root.create_child<MockWidget>("panel");

    SECTION("fetch_child returns matching child with exact type") {
        auto* found_widget = root.fetch_child<Widget>("button");
        REQUIRE(found_widget != nullptr);
        CHECK(found_widget == &widget_child);
        CHECK(found_widget->name() == "button");

        auto* found_mock = root.fetch_child<MockWidget>("panel");
        REQUIRE(found_mock != nullptr);
        CHECK(found_mock == &mock_child);
        CHECK(found_mock->name() == "panel");
    }

    SECTION("fetch_child returns base type pointer for derived child") {
        auto* found_as_base = root.fetch_child<Widget>("panel");
        REQUIRE(found_as_base != nullptr);
        CHECK(found_as_base == &mock_child);
    }

    SECTION("fetch_child returns nullptr when name not found") {
        CHECK(root.fetch_child<Widget>("non_existent") == nullptr);
    }

    SECTION("fetch_child returns nullptr on type mismatch") {
        CHECK(root.fetch_child<MockWidget>("button") == nullptr);
    }

    SECTION("fetch_child returns first matching child when names duplicate") {
        auto& first = root.create_child<Widget>("duplicate");
        auto& second = root.create_child<Widget>("duplicate");
        auto* found = root.fetch_child<Widget>("duplicate");
        REQUIRE(found != nullptr);
        CHECK(found == &first);
        CHECK(found != &second);
    }

    SECTION("fetch_child works on const widget") {
        const Widget& const_root = root;
        const auto* found_const = const_root.fetch_child<Widget>("button");
        REQUIRE(found_const != nullptr);
        CHECK(found_const == &widget_child);
        CHECK(found_const->name() == "button");

        CHECK(const_root.fetch_child<Widget>("non_existent") == nullptr);
        CHECK(const_root.fetch_child<MockWidget>("button") == nullptr);
    }

    SECTION("fetch_child does not search nested descendants") {
        mock_child.create_child<Widget>("nested");
        CHECK(root.fetch_child<Widget>("nested") == nullptr);
        CHECK(mock_child.fetch_child<Widget>("nested") != nullptr);
    }
}
