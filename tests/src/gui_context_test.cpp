#include "serenkai/base/glm_fmt.hpp"
#include "serenkai/gui/gui_context.hpp"
#include "serenkai/gui/widget.hpp"

#include <catch2/catch_test_macros.hpp>
#include <fmt/format.h>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

using namespace serenkai;

TEST_CASE("GuiContext window resize event handling and scaling",
          "[gui][context]") {
    GuiContext context(GuiConfig{nullptr, nullptr});

    SECTION("Invalid dimensions do not update logical size") {
        Widget::set_logical_window_size({100, 100});

        context.handle_window_resize_event(WindowResizeEvent{0, 720});
        CHECK(Widget::logical_window_size() == glm::ivec2{100, 100});

        context.handle_window_resize_event(WindowResizeEvent{1280, 0});
        CHECK(Widget::logical_window_size() == glm::ivec2{100, 100});

        context.handle_window_resize_event(WindowResizeEvent{-100, -100});
        CHECK(Widget::logical_window_size() == glm::ivec2{100, 100});
    }

    SECTION("720p resolution computes scale 2") {
        // (720 + 270) / 540 = 1, clamped to min scale 2
        // logical size: (1280 / 2, 720 / 2) = (640, 360)
        context.handle_window_resize_event(WindowResizeEvent{1280, 720});
        CHECK(Widget::logical_window_size() == glm::ivec2{640, 360});
    }

    SECTION("1080p resolution enforces minimum scale 3") {
        // 1920x1080: (1080 + 270) / 540 = 2, max(2, 3) = 3
        // logical size: (1920 / 3, 1080 / 3) = (640, 360)
        context.handle_window_resize_event(WindowResizeEvent{1920, 1080});
        CHECK(Widget::logical_window_size() == glm::ivec2{640, 360});
    }

    SECTION("High resolution scaling and clamp limits") {
        // 4K (3840x2160): (2160 + 270) / 540 = 4
        // logical size: (3840 / 4, 2160 / 4) = (960, 540)
        context.handle_window_resize_event(WindowResizeEvent{3840, 2160});
        CHECK(Widget::logical_window_size() == glm::ivec2{960, 540});

        // Extreme height clamped to max scale 8
        // 8000 / 8 = 1000, 10000 / 8 = 1250
        context.handle_window_resize_event(WindowResizeEvent{8000, 10000});
        CHECK(Widget::logical_window_size() == glm::ivec2{1000, 1250});
    }

    SECTION("Consecutive resize events with same scale update logical size") {
        // 1257x1530: (1530 + 270) / 540 = 3
        // logical: (1257 / 3, 1530 / 3) = (419, 510)
        context.handle_window_resize_event(WindowResizeEvent{1257, 1530});
        CHECK(Widget::logical_window_size() == glm::ivec2{419, 510});

        // Width changes in tiling window manager, scale stays 3
        // 1200x1530 -> logical: (1200 / 3, 1530 / 3) = (400, 510)
        context.handle_window_resize_event(WindowResizeEvent{1200, 1530});
        CHECK(Widget::logical_window_size() == glm::ivec2{400, 510});

        // Duplicate identical event does not alter logical size
        context.handle_window_resize_event(WindowResizeEvent{1200, 1530});
        CHECK(Widget::logical_window_size() == glm::ivec2{400, 510});
    }
}

TEST_CASE("GLM vector formatting via fmt", "[base][format]") {
    CHECK(fmt::format("{}", glm::ivec2{10, 20}) == "(10, 20)");
    CHECK(fmt::format("{}", glm::vec2{1.5F, -2.5F}) == "(1.5, -2.5)");
    CHECK(fmt::format("{}", glm::ivec3{1, 2, 3}) == "(1, 2, 3)");
}
