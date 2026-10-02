#include "serenkai/application/event.hpp"
#include "serenkai/application/input.hpp"
#include "serenkai/application/key.hpp"
#include "serenkai/scenes/scene.hpp"
#include "serenkai/scenes/scene_manager.hpp"

#include <SDL3/SDL_events.h>
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <memory>

using namespace serenkai;

TEST_CASE("SDL keyboard event translation", "[input]") {
    SECTION("Key press and repeat actions") {
        SDL_Event e{};
        e.type = SDL_EVENT_KEY_DOWN;
        e.key.key = SDLK_A;
        e.key.repeat = false;

        auto event = input::process_sdl_event(e);
        REQUIRE(event.has_value());
        const auto* key_event = std::get_if<KeyEvent>(&*event);
        REQUIRE(key_event != nullptr);
        CHECK(key_event->key == Key::A);
        CHECK(key_event->action == KeyAction::Press);

        e.key.repeat = true;
        event = input::process_sdl_event(e);
        REQUIRE(event.has_value());
        key_event = std::get_if<KeyEvent>(&*event);
        REQUIRE(key_event != nullptr);
        CHECK(key_event->key == Key::A);
        CHECK(key_event->action == KeyAction::Repeat);
    }

    SECTION("Key release action") {
        SDL_Event e{};
        e.type = SDL_EVENT_KEY_UP;
        e.key.key = SDLK_ESCAPE;

        auto event = input::process_sdl_event(e);
        REQUIRE(event.has_value());
        const auto* key_event = std::get_if<KeyEvent>(&*event);
        REQUIRE(key_event != nullptr);
        CHECK(key_event->key == Key::Escape);
        CHECK(key_event->action == KeyAction::Release);
    }

    SECTION("Key code mappings for representative categories") {
        auto translate = [](SDL_Keycode code) {
            SDL_Event e{};
            e.type = SDL_EVENT_KEY_DOWN;
            e.key.key = code;
            auto ev = input::process_sdl_event(e);
            REQUIRE(ev.has_value());
            const auto* k = std::get_if<KeyEvent>(&*ev);
            REQUIRE(k != nullptr);
            return k->key;
        };

        // Letter keys
        CHECK(translate(SDLK_Z) == Key::Z);
        CHECK(translate(SDLK_M) == Key::M);

        // Digits
        CHECK(translate(SDLK_0) == Key::Digit0);
        CHECK(translate(SDLK_9) == Key::Digit9);

        // Function keys
        CHECK(translate(SDLK_F1) == Key::F1);
        CHECK(translate(SDLK_F12) == Key::F12);

        // Control and navigation keys
        CHECK(translate(SDLK_SPACE) == Key::Space);
        CHECK(translate(SDLK_RETURN) == Key::Enter);
        CHECK(translate(SDLK_BACKSPACE) == Key::Backspace);
        CHECK(translate(SDLK_TAB) == Key::Tab);
        CHECK(translate(SDLK_HOME) == Key::Home);
        CHECK(translate(SDLK_END) == Key::End);
        CHECK(translate(SDLK_PAGEUP) == Key::PageUp);
        CHECK(translate(SDLK_PAGEDOWN) == Key::PageDown);
        CHECK(translate(SDLK_LEFT) == Key::Left);
        CHECK(translate(SDLK_RIGHT) == Key::Right);
        CHECK(translate(SDLK_UP) == Key::Up);
        CHECK(translate(SDLK_DOWN) == Key::Down);

        // Modifiers
        CHECK(translate(SDLK_LSHIFT) == Key::LeftShift);
        CHECK(translate(SDLK_RSHIFT) == Key::RightShift);
        CHECK(translate(SDLK_LCTRL) == Key::LeftCtrl);
        CHECK(translate(SDLK_RCTRL) == Key::RightCtrl);
        CHECK(translate(SDLK_LALT) == Key::LeftAlt);
        CHECK(translate(SDLK_RALT) == Key::RightAlt);
        CHECK(translate(SDLK_LGUI) == Key::LeftSuper);
        CHECK(translate(SDLK_RGUI) == Key::RightSuper);

        // Numpad
        CHECK(translate(SDLK_KP_0) == Key::Numpad0);
        CHECK(translate(SDLK_KP_PLUS) == Key::NumpadAdd);
        CHECK(translate(SDLK_KP_ENTER) == Key::NumpadEnter);
    }

    SECTION("Unknown key returns nullopt") {
        SDL_Event e{};
        e.type = SDL_EVENT_KEY_DOWN;
        e.key.key = static_cast<SDL_Keycode>(0x7FFFFFFF);

        CHECK_FALSE(input::process_sdl_event(e).has_value());
    }
}

TEST_CASE("SDL mouse event translation", "[input]") {
    SECTION("Mouse button events") {
        auto check_button = [](uint8_t btn, Key expected_key) {
            SDL_Event e_down{};
            e_down.type = SDL_EVENT_MOUSE_BUTTON_DOWN;
            e_down.button.button = btn;
            auto ev_down = input::process_sdl_event(e_down);
            REQUIRE(ev_down.has_value());
            const auto* k_down = std::get_if<KeyEvent>(&*ev_down);
            REQUIRE(k_down != nullptr);
            CHECK(k_down->key == expected_key);
            CHECK(k_down->action == KeyAction::Press);

            SDL_Event e_up{};
            e_up.type = SDL_EVENT_MOUSE_BUTTON_UP;
            e_up.button.button = btn;
            auto ev_up = input::process_sdl_event(e_up);
            REQUIRE(ev_up.has_value());
            const auto* k_up = std::get_if<KeyEvent>(&*ev_up);
            REQUIRE(k_up != nullptr);
            CHECK(k_up->key == expected_key);
            CHECK(k_up->action == KeyAction::Release);
        };

        check_button(SDL_BUTTON_LEFT, Key::MouseLeft);
        check_button(SDL_BUTTON_RIGHT, Key::MouseRight);
        check_button(SDL_BUTTON_MIDDLE, Key::MouseMiddle);
        check_button(SDL_BUTTON_X1, Key::MouseBack);
        check_button(SDL_BUTTON_X2, Key::MouseForward);
    }

    SECTION("Unknown mouse button returns nullopt") {
        SDL_Event e{};
        e.type = SDL_EVENT_MOUSE_BUTTON_DOWN;
        e.button.button = 255;

        CHECK_FALSE(input::process_sdl_event(e).has_value());
    }

    SECTION("Mouse motion within threshold") {
        SDL_Event e{};
        e.type = SDL_EVENT_MOUSE_MOTION;
        e.motion.x = 320.0F;
        e.motion.y = 240.0F;
        e.motion.xrel = 15.0F;
        e.motion.yrel = -8.0F;

        auto event = input::process_sdl_event(e);
        REQUIRE(event.has_value());
        const auto* motion = std::get_if<MouseMoveEvent>(&*event);
        REQUIRE(motion != nullptr);
        CHECK(motion->xpos == Catch::Approx(320.0F));
        CHECK(motion->ypos == Catch::Approx(240.0F));
        CHECK(motion->xrel == Catch::Approx(15.0F));
        CHECK(motion->yrel == Catch::Approx(-8.0F));
    }

    SECTION("Mouse motion exceeding threshold is filtered") {
        SDL_Event e{};
        e.type = SDL_EVENT_MOUSE_MOTION;
        e.motion.x = 320.0F;
        e.motion.y = 240.0F;
        e.motion.xrel = 205.0F;
        e.motion.yrel = 0.0F;

        CHECK_FALSE(input::process_sdl_event(e).has_value());

        e.motion.xrel = 0.0F;
        e.motion.yrel = -201.0F;
        CHECK_FALSE(input::process_sdl_event(e).has_value());
    }

    SECTION("Mouse wheel translation and flipping") {
        SDL_Event e{};
        e.type = SDL_EVENT_MOUSE_WHEEL;
        e.wheel.x = 2.0F;
        e.wheel.y = -3.0F;
        e.wheel.direction = SDL_MOUSEWHEEL_NORMAL;

        auto event = input::process_sdl_event(e);
        REQUIRE(event.has_value());
        const auto* wheel = std::get_if<MouseWheelEvent>(&*event);
        REQUIRE(wheel != nullptr);
        CHECK(wheel->xoffset == Catch::Approx(2.0F));
        CHECK(wheel->yoffset == Catch::Approx(-3.0F));

        e.wheel.direction = SDL_MOUSEWHEEL_FLIPPED;
        event = input::process_sdl_event(e);
        REQUIRE(event.has_value());
        wheel = std::get_if<MouseWheelEvent>(&*event);
        REQUIRE(wheel != nullptr);
        CHECK(wheel->xoffset == Catch::Approx(-2.0F));
        CHECK(wheel->yoffset == Catch::Approx(3.0F));
    }
}

TEST_CASE("SDL window and system event translation", "[input]") {
    SECTION("Window resize event") {
        SDL_Event e{};
        e.type = SDL_EVENT_WINDOW_RESIZED;
        e.window.data1 = 1280;
        e.window.data2 = 720;

        auto event = input::process_sdl_event(e);
        REQUIRE(event.has_value());
        const auto* resize = std::get_if<WindowResizeEvent>(&*event);
        REQUIRE(resize != nullptr);
        CHECK(resize->width == 1280);
        CHECK(resize->height == 720);
    }

    SECTION("Framebuffer resize event") {
        SDL_Event e{};
        e.type = SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED;
        e.window.data1 = 2560;
        e.window.data2 = 1440;

        auto event = input::process_sdl_event(e);
        REQUIRE(event.has_value());
        const auto* fb = std::get_if<FrameBufferResizeEvent>(&*event);
        REQUIRE(fb != nullptr);
        CHECK(fb->width == 2560);
        CHECK(fb->height == 1440);
    }

    SECTION("Focus change events") {
        SDL_Event e_gain{};
        e_gain.type = SDL_EVENT_WINDOW_FOCUS_GAINED;

        auto event_gain = input::process_sdl_event(e_gain);
        REQUIRE(event_gain.has_value());
        const auto* focus_gain =
            std::get_if<WindowFocusChangeEvent>(&*event_gain);
        REQUIRE(focus_gain != nullptr);
        CHECK(focus_gain->focus);

        SDL_Event e_lost{};
        e_lost.type = SDL_EVENT_WINDOW_FOCUS_LOST;

        auto event_lost = input::process_sdl_event(e_lost);
        REQUIRE(event_lost.has_value());
        const auto* focus_lost =
            std::get_if<WindowFocusChangeEvent>(&*event_lost);
        REQUIRE(focus_lost != nullptr);
        CHECK_FALSE(focus_lost->focus);
    }

    SECTION("Text input event") {
        SDL_Event e{};
        e.type = SDL_EVENT_TEXT_INPUT;
        e.text.text = "Serenkai";

        auto event = input::process_sdl_event(e);
        REQUIRE(event.has_value());
        const auto* text = std::get_if<TextInputEvent>(&*event);
        REQUIRE(text != nullptr);
        CHECK(text->text == "Serenkai");
    }

    SECTION("Quit event") {
        SDL_Event e{};
        e.type = SDL_EVENT_QUIT;

        auto event = input::process_sdl_event(e);
        REQUIRE(event.has_value());
        CHECK(std::holds_alternative<QuitEvent>(*event));
    }

    SECTION("Unsupported event type returns nullopt") {
        SDL_Event e{};
        e.type = SDL_EVENT_CLIPBOARD_UPDATE;

        CHECK_FALSE(input::process_sdl_event(e).has_value());
    }
}

namespace {
/// @brief Test scene tracking dispatched events to verify default dispatching.
class EventSpyScene : public Scene {
public:
    struct Stats {
        int key_event_count{0};
        int mouse_move_count{0};
        int mouse_wheel_count{0};
        int resize_count{0};
        int text_input_count{0};
    };

    explicit EventSpyScene(std::shared_ptr<Stats> stats)
        : m_stats(std::move(stats)) {}

    void update(float) override {}
    void render(Renderer&) override {}

protected:
    bool handle_key_event(const KeyEvent&) override {
        if (m_stats) {
            ++m_stats->key_event_count;
        }
        return true;
    }

    bool handle_mouse_move_event(const MouseMoveEvent&) override {
        if (m_stats) {
            ++m_stats->mouse_move_count;
        }
        return true;
    }

    bool handle_mouse_wheel_event(const MouseWheelEvent&) override {
        if (m_stats) {
            ++m_stats->mouse_wheel_count;
        }
        return true;
    }

    bool handle_window_resize_event(const WindowResizeEvent&) override {
        if (m_stats) {
            ++m_stats->resize_count;
        }
        return true;
    }

    bool handle_text_input_event(const TextInputEvent&) override {
        if (m_stats) {
            ++m_stats->text_input_count;
        }
        return true;
    }

private:
    std::shared_ptr<Stats> m_stats;
};

/// @brief Scene manager producing EventSpyScene instances for tests.
class EventSpySceneManager : public SceneManager {
public:
    std::shared_ptr<EventSpyScene::Stats> stats =
        std::make_shared<EventSpyScene::Stats>();

protected:
    std::unique_ptr<Scene> create_scene(SceneType) override {
        return std::make_unique<EventSpyScene>(stats);
    }
};
} // namespace

TEST_CASE("Scene and SceneManager event dispatching", "[scene]") {
    EventSpySceneManager manager;

    SECTION("Empty scene manager returns false on event") {
        CHECK(manager.empty());
        Event ev = QuitEvent{};
        CHECK_FALSE(manager.handle_event(ev));
    }

    SECTION("SceneManager forwards event to current scene") {
        manager.request_push(SceneType::Title);
        manager.update(0.016F);
        REQUIRE_FALSE(manager.empty());

        Event key_ev = KeyEvent{Key::Space, KeyAction::Press};
        CHECK(manager.handle_event(key_ev));
        CHECK(manager.stats->key_event_count == 1);
    }

    SECTION("Scene default handle_event routes to specific protected hooks") {
        auto stats = std::make_shared<EventSpyScene::Stats>();
        EventSpyScene scene(stats);

        CHECK(scene.handle_event(KeyEvent{Key::Space, KeyAction::Press}));
        CHECK(stats->key_event_count == 1);

        CHECK(scene.handle_event(MouseMoveEvent{10.0F, 20.0F, 1.0F, 2.0F}));
        CHECK(stats->mouse_move_count == 1);

        CHECK(scene.handle_event(MouseWheelEvent{0.0F, 1.0F}));
        CHECK(stats->mouse_wheel_count == 1);

        CHECK(scene.handle_event(WindowResizeEvent{800, 600}));
        CHECK(stats->resize_count == 1);

        CHECK(scene.handle_event(TextInputEvent{"a"}));
        CHECK(stats->text_input_count == 1);

        // Unhandled events fall through to default false
        CHECK_FALSE(scene.handle_event(QuitEvent{}));
        CHECK_FALSE(scene.handle_event(WindowFocusChangeEvent{true}));
    }
}
