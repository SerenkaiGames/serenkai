#include "serenkai/application/window_manager.hpp"

#include <SDL3/SDL_events.h>
#include <SDL3/SDL_properties.h>
#include <SDL3/SDL_video.h>
#include <spdlog/spdlog.h>
#include <stdexcept>
#include <utility>
namespace serenkai {
WindowManager::WindowManager(WindowConfig config)
    : m_config(std::move(config)) {
    SDL_PropertiesID props = SDL_CreateProperties();
    SDL_SetStringProperty(props, SDL_PROP_WINDOW_CREATE_TITLE_STRING,
                          "Serenkai");
    SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_WIDTH_NUMBER,
                          m_config.width);
    SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_HEIGHT_NUMBER,
                          m_config.height);

    SDL_SetBooleanProperty(props, SDL_PROP_WINDOW_CREATE_RESIZABLE_BOOLEAN,
                           true);
    SDL_SetBooleanProperty(
        props, SDL_PROP_WINDOW_CREATE_HIGH_PIXEL_DENSITY_BOOLEAN, true);

    m_window = SDL_CreateWindowWithProperties(props);

    SDL_DestroyProperties(props);

    if (!m_window) {
        throw std::runtime_error(fmt::format(
            "SDL_CreateWindowWithProperties failed: {}", SDL_GetError()));
    }

    SDL_SetWindowPosition(m_window, SDL_WINDOWPOS_CENTERED,
                          SDL_WINDOWPOS_CENTERED);

    set_fullscreen(m_config.mode);
}

WindowManager::~WindowManager() {
    if (m_window) {
        SDL_DestroyWindow(m_window);
    }
}

bool WindowManager::set_fullscreen(FullscreenMode mode) {

    switch (mode) {
    case FullscreenMode::Windowed: {
        if (!SDL_SetWindowFullscreen(m_window, false)) {
            return false;
        }

        if (!SDL_SetWindowBordered(m_window, true)) {
            return false;
        }
        SDL_SetWindowPosition(m_window, m_windowed_xpos, m_windowed_ypos);
        SDL_SetWindowSize(m_window, m_windowed_width, m_windowed_height);
    } break;
    case FullscreenMode::Fullscreen: {
        SDL_GetWindowPosition(m_window, &m_windowed_xpos, &m_windowed_ypos);
        SDL_GetWindowSize(m_window, &m_windowed_width, &m_windowed_height);
        SDL_SetWindowFullscreen(m_window, true);
    } break;
    case FullscreenMode::FullscreenBorderless: {
        // If the window is maximized, restore it first.
        if (SDL_GetWindowFlags(m_window) & SDL_WINDOW_MAXIMIZED) {
            SDL_RestoreWindow(m_window);
        }

        SDL_GetWindowPosition(m_window, &m_windowed_xpos, &m_windowed_ypos);

        SDL_GetWindowSize(m_window, &m_windowed_width, &m_windowed_height);

        auto windowed_display = SDL_GetDisplayForWindow(m_window);

        SDL_Rect display_bounds{};
        if (!SDL_GetDisplayBounds(windowed_display, &display_bounds)) {

            spdlog::error("SDL_GetDisplayBounds failed: {}", SDL_GetError());
            return false;
        }

        SDL_SetWindowBordered(m_window, false);

        SDL_SetWindowPosition(m_window, display_bounds.x, display_bounds.y);

        SDL_SetWindowSize(m_window, display_bounds.w, display_bounds.h);
    } break;
    }

    SDL_RaiseWindow(m_window);
    /*
    int w = 0, h = 0;
    SDL_GetWindowSize(m_window, &w, &h);

    SDL_Event event{};
    event.type = SDL_EVENT_WINDOW_RESIZED;
    event.window.data1 = w;
    event.window.data2 = h;
    event.window.windowID = SDL_GetWindowID(m_window);

    SDL_PushEvent(&event);
     */

    // Update IME candidate window position
    SDL_Rect ime_rect{0, 0, 1, 1};

    SDL_SetTextInputArea(m_window, &ime_rect, 0);

    return true;
}

SDL_Window* WindowManager::get_window() const { return m_window; }
} // namespace serenkai
