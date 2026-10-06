#include "serenkai/application/application.hpp"

#include "serenkai/application/event.hpp"
#include "serenkai/application/input.hpp"
#include "serenkai/application/window_manager.hpp"
#include "serenkai/gui/gui_context.hpp"
#include "serenkai/gui/widget_parser.hpp"
#include "serenkai/render/renderer.hpp"
#include "serenkai/resource/asset_manager.hpp"
#include "serenkai/resource/directory_source.hpp"
#include "serenkai/resource/font_manager.hpp"
#include "serenkai/resource/image_loader.hpp"
#include "serenkai/resource/texture_manager.hpp"
#include "serenkai/scenes/scene.hpp"
#include "serenkai/scenes/scene_manager.hpp"
#include "serenkai/script/script_engine.hpp"

#include <SDL3/SDL_error.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_timer.h>
#include <SDL3/SDL_version.h>
#include <fmt/format.h>
#include <glm/ext/vector_float2.hpp>
#include <memory>
#include <spdlog/spdlog.h>
#include <stdexcept>
#include <variant>

namespace serenkai {

namespace {
void init_sdl() {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        throw std::runtime_error(
            fmt::format("Failed to initialize SDL video subsystem, error: {}",
                        SDL_GetError()));
    }
}

void cleanup_sdl() {
    SDL_Quit();
    spdlog::info("Application quit");
}

} // namespace

Application::Application() {

#ifndef NDEBUG
    spdlog::set_level(spdlog::level::debug);
#endif

    m_asset_manager = std::make_unique<AssetManager>();

    auto source = std::make_shared<DirectorySource>("./assets");
    m_asset_manager->merge_source(source);

    m_font_manager = std::make_unique<FontManager>(m_asset_manager.get());

    int linked = SDL_GetVersion();
    spdlog::info("Linked SDL version: {}.{}.{}", SDL_VERSIONNUM_MAJOR(linked),
                 SDL_VERSIONNUM_MINOR(linked), SDL_VERSIONNUM_MICRO(linked));
    m_sdl_wrapper = std::make_unique<SdlGuard>(init_sdl, cleanup_sdl);

    m_window_manager = std::make_unique<WindowManager>(WindowConfig{});

    m_renderer = std::make_unique<Renderer>(
        RendererConfig{true, m_window_manager->get_window()});
    m_texture_manager = std::make_unique<TextureManager>(
        m_asset_manager.get(), m_renderer->get_sdl_renderer());

    m_gui_context = std::make_unique<GuiContext>(
        GuiConfig{m_renderer.get(), m_texture_manager.get()});

    m_scene_manager = std::make_unique<SceneManager>();

    m_widget_parser = std::make_unique<WidgetParser>(WidgetParserConfig{
        m_asset_manager.get(), m_font_manager.get(), m_texture_manager.get()});

    m_widget_parser->register_callback("on_exit_game",
                                       [this]() { m_running = false; });
    m_widget_parser->register_callback(
        "on_pop_scene", [this]() { m_scene_manager->request_pop(); });
    m_widget_parser->register_callback("on_change_title_scene", [this]() {
        m_scene_manager->request_change(SceneType::Title);
    });
    m_widget_parser->register_callback("on_change_game_scene", [this]() {
        m_scene_manager->request_change(SceneType::Game);
    });
    m_script_engine = std::make_unique<ScriptEngine>(m_asset_manager.get());

    auto window_size = m_window_manager->get_window_size();
    m_gui_context->handle_window_resize_event(
        WindowResizeEvent{window_size.x, window_size.y});

    m_script_engine->load("serenkai:scripts/main.luau");
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
        if (auto event = input::process_sdl_event(m_event)) {
            dispatch_event(*event);
        }
    }

    update(dt);
    render();
}

void Application::update(float dt) {
    // m_scene_manager->update(dt);
    m_script_engine->call<>("serenkai:scripts/main.luau", "on_update", dt);
}
void Application::render() {
    m_renderer->clear();
    // m_scene_manager->render(m_gui_context.get());
    m_renderer->present();
}

void Application::dispatch_event(Event& e) {
    if (auto _ = std::get_if<QuitEvent>(&e)) {
        m_running = false;
        return;
    }

    if (auto event = std::get_if<WindowResizeEvent>(&e)) {
        m_gui_context->handle_window_resize_event(*event);
    }

    if (auto event = std::get_if<MouseMoveEvent>(&e)) {
        if (m_gui_context) {

            auto logical = m_gui_context->to_logical_coord(
                glm::vec2{event->xpos, event->ypos});

            event->logical_x = logical.x;
            event->logical_y = logical.y;
        }
    }

    if (m_scene_manager->handle_event(e)) {
        return;
    }
}

} // namespace serenkai
