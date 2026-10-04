#include "serenkai/application/event.hpp"
#include "serenkai/gui/button.hpp"
#include "serenkai/gui/rect.hpp"
#include "serenkai/gui/widget.hpp"

#include <catch2/catch_test_macros.hpp>
#include <memory>

using namespace serenkai;

TEST_CASE("Button default state and property setters", "[gui][button]") {
    Button button("test_btn", nullptr);

    CHECK(button.name() == "test_btn");
    CHECK(button.is_enabled());
    CHECK_FALSE(button.is_hovered());
    CHECK(button.size() == glm::ivec2{0, 0});
    CHECK(button.parent() == nullptr);

    SECTION("Enable state mutation") {
        button.set_enabled(false);
        CHECK_FALSE(button.is_enabled());

        button.set_enabled(true);
        CHECK(button.is_enabled());
    }
}

TEST_CASE("Button auto-sizing from children", "[gui][button]") {
    Button button("sized_btn", nullptr);

    auto rect1 = std::make_unique<Rect>("child1", nullptr);
    rect1->set_size({100, 40});

    auto rect2 = std::make_unique<Rect>("child2", nullptr);
    rect2->set_size({60, 80});

    button.add_child(std::move(rect1));
    button.add_child(std::move(rect2));

    CHECK(button.size() == glm::ivec2{0, 0});

    // on_update triggers measure_from_children
    button.update(0.016f);
    CHECK(button.size() == glm::ivec2{100, 80});
}

TEST_CASE("Button mouse move and hover hit-testing", "[gui][button]") {
    Widget::set_logical_window_size({800, 600});

    Button button("hover_btn", nullptr);
    auto child = std::make_unique<Rect>("bg", nullptr);
    child->set_size({120, 60});
    button.add_child(std::move(child));
    button.update(0.016f);

    REQUIRE(button.size() == glm::ivec2{120, 60});

    SECTION("Hover inside button bounds") {
        MouseMoveEvent move_in{60.0f, 30.0f, 0.0f, 0.0f};
        button.handle_mouse_move_event(move_in);
        CHECK(button.is_hovered());
    }

    SECTION("Move outside button bounds clears hover") {
        MouseMoveEvent move_in{60.0f, 30.0f, 0.0f, 0.0f};
        button.handle_mouse_move_event(move_in);
        REQUIRE(button.is_hovered());

        MouseMoveEvent move_out{200.0f, 30.0f, 0.0f, 0.0f};
        button.handle_mouse_move_event(move_out);
        CHECK_FALSE(button.is_hovered());
    }

    SECTION("Disabled button cannot be hovered") {
        button.set_enabled(false);

        MouseMoveEvent move_in{60.0f, 30.0f, 0.0f, 0.0f};
        button.handle_mouse_move_event(move_in);
        CHECK_FALSE(button.is_hovered());
    }
}

TEST_CASE("Button click event handling", "[gui][button]") {
    Widget::set_logical_window_size({800, 600});

    Button button("click_btn", nullptr);
    auto child = std::make_unique<Rect>("bg", nullptr);
    child->set_size({100, 50});
    button.add_child(std::move(child));
    button.update(0.016f);

    int click_count = 0;
    button.set_clicked([&]() { ++click_count; });

    const KeyEvent left_press{Key::MouseLeft, KeyAction::Press};
    const KeyEvent left_release{Key::MouseLeft, KeyAction::Release};
    const KeyEvent right_press{Key::MouseRight, KeyAction::Press};

    SECTION("Click when hovered triggers callback and consumes event") {
        MouseMoveEvent move_in{50.0f, 25.0f, 0.0f, 0.0f};
        button.handle_mouse_move_event(move_in);
        REQUIRE(button.is_hovered());

        CHECK(button.handle_key_event(left_press));
        CHECK(click_count == 1);
    }

    SECTION(
        "Click when not hovered does not trigger callback nor consume event") {
        CHECK_FALSE(button.is_hovered());
        CHECK_FALSE(button.handle_key_event(left_press));
        CHECK(click_count == 0);
    }

    SECTION("Non-left-click or non-press actions are ignored") {
        MouseMoveEvent move_in{50.0f, 25.0f, 0.0f, 0.0f};
        button.handle_mouse_move_event(move_in);
        REQUIRE(button.is_hovered());

        CHECK_FALSE(button.handle_key_event(left_release));
        CHECK_FALSE(button.handle_key_event(right_press));
        CHECK(click_count == 0);
    }

    SECTION(
        "Disabled hovered button blocks click but does not invoke callback") {
        MouseMoveEvent move_in{50.0f, 25.0f, 0.0f, 0.0f};
        button.handle_mouse_move_event(move_in);
        REQUIRE(button.is_hovered());

        button.set_enabled(false);
        // Consumes event to prevent penetration, but callback is not called
        CHECK(button.handle_key_event(left_press));
        CHECK(click_count == 0);
    }
}

TEST_CASE("Hierarchical event dispatching across multiple buttons",
          "[gui][button]") {
    Widget::set_logical_window_size({800, 600});

    Widget root("root", nullptr);

    auto btn_a = std::make_unique<Button>("btn_a", nullptr);
    btn_a->set_offset({0, 0});
    auto rect_a = std::make_unique<Rect>("rect_a", nullptr);
    rect_a->set_size({100, 50});
    btn_a->add_child(std::move(rect_a));

    auto btn_b = std::make_unique<Button>("btn_b", nullptr);
    btn_b->set_offset({150, 0});
    auto rect_b = std::make_unique<Rect>("rect_b", nullptr);
    rect_b->set_size({100, 50});
    btn_b->add_child(std::move(rect_b));

    Button* p_btn_a = btn_a.get();
    Button* p_btn_b = btn_b.get();

    root.add_child(std::move(btn_a));
    root.add_child(std::move(btn_b));
    root.update(0.016f);

    int clicked_a = 0;
    int clicked_b = 0;
    p_btn_a->set_clicked([&]() { ++clicked_a; });
    p_btn_b->set_clicked([&]() { ++clicked_b; });

    const KeyEvent left_press{Key::MouseLeft, KeyAction::Press};

    // Move to Button A
    MouseMoveEvent move_to_a{50.0f, 25.0f, 0.0f, 0.0f};
    root.handle_mouse_move_event(move_to_a);
    CHECK(p_btn_a->is_hovered());
    CHECK_FALSE(p_btn_b->is_hovered());

    CHECK(root.handle_key_event(left_press));
    CHECK(clicked_a == 1);
    CHECK(clicked_b == 0);

    // Move to Button B
    MouseMoveEvent move_to_b{200.0f, 25.0f, 0.0f, 0.0f};
    root.handle_mouse_move_event(move_to_b);
    CHECK_FALSE(p_btn_a->is_hovered());
    CHECK(p_btn_b->is_hovered());

    CHECK(root.handle_key_event(left_press));
    CHECK(clicked_a == 1);
    CHECK(clicked_b == 1);
}
