#pragma once

#include "serenkai/application/event.hpp"
#include "serenkai/application/window_manager.hpp"
#include "serenkai/base/raii.hpp"
#include "serenkai/gui/gui_context.hpp"
#include "serenkai/gui/widget_parser.hpp"
#include "serenkai/render/renderer.hpp"
#include "serenkai/resource/asset_manager.hpp"
#include "serenkai/resource/font_manager.hpp"
#include "serenkai/scenes/scene_manager.hpp"

#include <cstdint>
#include <functional>
#include <memory>

namespace serenkai {

/// @brief Game application
///
/// Manages the lifecycle of the entire game.
/// Initializes the whole application in the constructor.
/// Destroys it in the destructor.
/// @note May throw exceptions.
class Application {
public:
    struct DeltaTime {
        uint64_t last_tick_ns = 0;
        uint64_t current_tick_ns = 0;
        double dt() const {
            double delta = static_cast<double>(current_tick_ns - last_tick_ns) /
                           1'000'000'000.0;
            return std::min(delta, 0.1); // Prevent a spiral of death caused by
                                         // an excessively large delta time
        }
    };

    Application();
    ~Application();

    bool is_running() const;

    void run();
    void step(float dt);

private:
    using SdlGuard = RaiiGuard<std::function<void()>, std::function<void()>>;
    // Must be declared first to ensure it is destroyed last.
    std::unique_ptr<AssetManager> m_asset_manager;
    std::unique_ptr<FontManager> m_font_manager;
    std::unique_ptr<SdlGuard> m_sdl_wrapper;
    std::unique_ptr<WindowManager> m_window_manager;
    std::unique_ptr<Renderer> m_renderer;
    std::unique_ptr<GuiContext> m_gui_context;
    std::unique_ptr<SceneManager> m_scene_manager;
    std::unique_ptr<WidgetParser> m_widget_parser;

    bool m_running = true;
    SDL_Event m_event{};

    DeltaTime m_delta_time_ns;

    void render();
    void update(float dt);
    void dispatch_event(Event& e);
};
} // namespace serenkai
