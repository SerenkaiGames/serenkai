#include "serenkai/gui/anchor.hpp"
#include "serenkai/gui/widget.hpp"

#include <catch2/catch_test_macros.hpp>
#include <memory>

using namespace serenkai;

namespace {

/// @brief Mock widget to verify update and render lifecycle invocations.
class MockWidget : public Widget {
public:
    explicit MockWidget(Widget* parent = nullptr) : Widget(parent) {}

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
    Widget widget(nullptr);

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
    Widget widget(nullptr);
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

    Widget root(nullptr);
    root.set_size({400, 300});
    root.set_anchor(Anchor::TopLeft);
    root.set_offset({100, 50});
    // root.pos() should be {100, 50}
    REQUIRE(root.pos() == glm::ivec2{100, 50});

    SECTION("Child positioned relative to parent") {
        auto& child = root.create_child<Widget>();
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
        auto& child = root.create_child<Widget>();
        child.set_size({200, 200});
        child.set_anchor(Anchor::TopLeft);
        child.set_offset({20, 20});
        // child.pos() == {120, 70}

        auto& grandchild = child.create_child<Widget>();
        grandchild.set_size({50, 50});
        grandchild.set_anchor(Anchor::BottomRight);
        grandchild.set_offset({5, 5});

        // grandchild pos = child.pos() + (200 - 50, 200 - 50) + (5, 5)
        //                = {120 + 150 + 5, 70 + 150 + 5} = {275, 225}
        CHECK(grandchild.pos() == glm::ivec2{275, 225});
    }
}

TEST_CASE("Child widget management and reparenting", "[gui][widget]") {
    Widget root(nullptr);
    REQUIRE(root.children().empty());

    SECTION("create_child binds parent and stores child") {
        auto& child = root.create_child<Widget>();
        CHECK(child.parent() == &root);
        REQUIRE(root.children().size() == 1);
        CHECK(root.children()[0].get() == &child);
    }

    SECTION("add_child transfers ownership and assigns parent") {
        auto standalone = std::make_unique<Widget>(nullptr);
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
    MockWidget root(nullptr);
    auto& child = root.create_child<MockWidget>();
    auto& grandchild = child.create_child<MockWidget>();

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
