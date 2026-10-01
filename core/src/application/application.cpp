#include "serenkai/application/application.hpp"

#include "serenkai/application/window_manager.hpp"
#include "serenkai/render/renderer.hpp"

#include <SDL3/SDL_error.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_version.h>
#include <fmt/format.h>
#include <memory>
#include <spdlog/spdlog.h>
#include <stdexcept>

Application::Application() {
    int linked = SDL_GetVersion();

    spdlog::info("Linked SDL version: {}.{}.{}", SDL_VERSIONNUM_MAJOR(linked),
                 SDL_VERSIONNUM_MINOR(linked), SDL_VERSIONNUM_MICRO(linked));

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        throw std::runtime_error(
            fmt::format("Fail to initialize SDL video subsystem, error: {}",
                        SDL_GetError()));
    }

    m_window_manager = std::make_unique<WindowManager>(WindowConfig{});
    m_renderer = std::make_unique<Renderer>(m_window_manager->get_window(),
                                            RendererConfig{});
}

Application::~Application() {
    SDL_Quit();
    spdlog::info("Application quitted");
}

void Application::run() {

    spdlog::info("Started running...");

    bool running = true;
    SDL_Event event{};

    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = false;
            }
        }
        update();
        render();
    }
}

void Application::update() {}
void Application::render() {
    m_renderer->clear();

    m_renderer->present();
}