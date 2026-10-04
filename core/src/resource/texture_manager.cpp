#include "serenkai/resource/texture_manager.hpp"

#include "serenkai/base/assert.hpp"
#include "serenkai/base/raii.hpp"
#include "serenkai/resource/asset_manager.hpp"
#include "serenkai/resource/image_loader.hpp"
#include "serenkai/resource/resource_location.hpp"

#include <memory>
#include <spdlog/spdlog.h>

namespace serenkai {
TextureManager::TextureManager(AssetManager* asset_manager)
    : m_image_loader(std::make_unique<ImageLoader>(asset_manager)) {}

TextureManager::~TextureManager() { clear(); }

void TextureManager::clear() { m_textures.clear(); }

void TextureManager::init_renderer(SDL_Renderer* renderer) {
    m_renderer = renderer;
}

SDL_Texture* TextureManager::get(std::string_view loc) {
    auto resource = ResourceLocation::parse(loc);
    if (!resource) {
        return nullptr;
    }

    auto it = m_textures.find(*resource);
    if (it != m_textures.end()) {
        return it->second.get();
    }
    if (!m_renderer) {
        SE_ASSERT(false);
        return nullptr;
    }
    auto image = m_image_loader->load(loc);

    const int pitch = image->width * 4;

    SDL_Surface* surface = SDL_CreateSurfaceFrom(image->width, image->height,
                                                 SDL_PIXELFORMAT_RGBA32,
                                                 (void*)image->data, pitch);
    RaiiGuard surface_guard{[]() {},
                            [surface]() { SDL_DestroySurface(surface); }};
    if (!surface) {
        spdlog::error("Failed to create SDL surface {}", loc);
        m_textures.try_emplace(*resource, nullptr);
        return nullptr;
    }

    SDL_Texture* texture = SDL_CreateTextureFromSurface(m_renderer, surface);

    if (!texture) {
        spdlog::error("Failed to create SDL texture {}", loc);
    }
    // Also insert failed textures to prevent repeated loading.
    m_textures.try_emplace(*resource, texture);
    return texture;
}

} // namespace serenkai