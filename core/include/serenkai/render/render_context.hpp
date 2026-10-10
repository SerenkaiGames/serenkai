#pragma once

namespace serenkai {
class Renderer;
class TextureManager;
class GuiContext;

struct RenderContext {
    Renderer* renderer = nullptr;
    TextureManager* textures = nullptr;
    GuiContext* gui = nullptr;
};
} // namespace serenkai