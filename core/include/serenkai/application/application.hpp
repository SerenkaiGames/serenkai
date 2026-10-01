#pragma once

#include "serenkai/application/window_manager.hpp"
#include "serenkai/render/renderer.hpp"

#include <memory>
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

    Application();
    ~Application();

    void run();

private:
    // Must be declared first to ensure it is destroyed last.
    std::unique_ptr<SdlWrapper> m_sdl_wrapper;
    std::unique_ptr<WindowManager> m_window_manager;
    std::unique_ptr<Renderer> m_renderer;

    void render();
    void update();
};