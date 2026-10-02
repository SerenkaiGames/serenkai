#pragma once

#include "serenkai/application/window_manager.hpp"
#include "serenkai/render/renderer.hpp"
#include "serenkai/scenes/scene_manager.hpp"

#include <cstdint>
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
    struct SdlWrapper {
        SdlWrapper();
        ~SdlWrapper();
    };

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
    // Must be declared first to ensure it is destroyed last.
    std::unique_ptr<SdlWrapper> m_sdl_wrapper;
    std::unique_ptr<WindowManager> m_window_manager;
    std::unique_ptr<Renderer> m_renderer;
    std::unique_ptr<SceneManager> m_scene_manager;

    bool m_running = true;
    SDL_Event m_event{};

    DeltaTime m_delta_time_ns;

    void render();
    void update(float dt);
};
} // namespace serenkai
