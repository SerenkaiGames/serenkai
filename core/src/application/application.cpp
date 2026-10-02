#include "serenkai/application/application.hpp"

#include "serenkai/application/window_manager.hpp"
#include "serenkai/render/renderer.hpp"
#include "serenkai/scenes/scene_manager.hpp"

#include <SDL3/SDL_error.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_timer.h>
#include <SDL3/SDL_version.h>
#include <fmt/format.h>
#include <memory>
#include <spdlog/spdlog.h>
#include <stdexcept>

namespace serenkai {

Application::SdlWrapper::SdlWrapper() {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        throw std::runtime_error(
            fmt::format("Failed to initialize SDL video subsystem, error: {}",
                        SDL_GetError()));
    }
}
Application::SdlWrapper::~SdlWrapper() {
    SDL_Quit();
    spdlog::info("Application quit");
}

Application::Application() {
    int linked = SDL_GetVersion();

    spdlog::info("Linked SDL version: {}.{}.{}", SDL_VERSIONNUM_MAJOR(linked),
                 SDL_VERSIONNUM_MINOR(linked), SDL_VERSIONNUM_MICRO(linked));
    m_sdl_wrapper = std::make_unique<SdlWrapper>();
    m_window_manager = std::make_unique<WindowManager>(WindowConfig{});
    m_renderer = std::make_unique<Renderer>(m_window_manager->get_window(),
                                            RendererConfig{});
    m_scene_manager = std::make_unique<SceneManager>();
}

Application::~Application() {}

bool Application::is_running() const { return m_running; }

void Application::run() {

    spdlog::info("Started running...");
    // Prevent first-frame delta time spikes
    m_delta_time_ns.last_tick_ns = SDL_GetTicksNS();
    while (is_running()) {

        m_delta_time_ns.current_tick_ns = SDL_GetTicksNS();
        double dt = m_delta_time_ns.dt();
        m_delta_time_ns.last_tick_ns = m_delta_time_ns.current_tick_ns;

        step(static_cast<float>(dt));
    }
}

void Application::step(float dt) {
    while (SDL_PollEvent(&m_event)) {
        if (m_event.type == SDL_EVENT_QUIT) {
            m_running = false;
        }
    }
    update(dt);
    render();
}

void Application::update(float) {}
void Application::render() {
    m_renderer->clear();

    m_renderer->present();
}
} // namespace serenkai
