#pragma once

#include <SDL3/SDL_video.h>

enum class FullscreenMode { Windowed, Fullscreen, FullscreenBorderless };

struct WindowConfig {

    FullscreenMode mode{FullscreenMode::Windowed};

    int width{1280};
    int height{720};
};

/// @brief Class for managing windows.
///
/// Automatically handles the lifecycle of the SDL window:
/// - Creates the window on construction
/// - Destroys the window on destruction
///
/// @note Please use smart pointers to manage the lifecycle, and ensure it
///       is called after the SDL video subsystem has been fully initialized.
class WindowManager {
public:
    WindowManager(const WindowManager&) = delete;
    WindowManager(WindowManager&&) = delete;
    WindowManager& operator=(const WindowManager&) = delete;
    WindowManager& operator=(WindowManager&&) = delete;

    WindowManager(WindowConfig config);
    ~WindowManager();

    bool set_fullscreen(FullscreenMode mode);

    SDL_Window* get_window() const;

private:
    const WindowConfig m_config;
    SDL_Window* m_window{nullptr};

    // Remember windowed state
    int m_windowed_xpos{0};
    int m_windowed_ypos{0};
    int m_windowed_width{1280};
    int m_windowed_height{720};
};
