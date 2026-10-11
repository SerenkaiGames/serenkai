#pragma once

namespace serenkai {
class AssetManager;
class TextureManager;
class FontManager;
class Renderer;
class WindowManager;
class ScriptEngine;
class GuiContext;

/// @brief Global-level context that aggregates core services.
struct AppContext {
    AssetManager* assets = nullptr;
    TextureManager* textures = nullptr;
    FontManager* fonts = nullptr;
    Renderer* renderer = nullptr;
    WindowManager* window = nullptr;
    ScriptEngine* script = nullptr;
    GuiContext* gui = nullptr;
};

} // namespace serenkai