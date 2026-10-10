#pragma once

#include "serenkai/render/texture.hpp"
#include "serenkai/resource/image_loader.hpp"
#include "serenkai/resource/resource_location.hpp"

#include <SDL3/SDL_render.h>
#include <glm/ext/vector_int2.hpp>
#include <memory>
#include <string_view>
#include <unordered_map>
namespace serenkai {
class AssetManager;
/// @brief Texture manager class
///
/// Load and manage textures through this class.
/// Use nearest-neighbor scaling for the texture.
class TextureManager {
public:
    TextureManager(const TextureManager&) = delete;
    TextureManager(TextureManager&&) = delete;
    TextureManager& operator=(const TextureManager&) = delete;
    TextureManager& operator=(TextureManager&&) = delete;

    explicit TextureManager(AssetManager* asset_manager,
                            SDL_Renderer* renderer);
    ~TextureManager();

    SDL_Texture* get(std::string_view loc);
    SDL_Texture* get(const ResourceLocation& loc);

    glm::ivec2 measure_size(std::string_view loc);

    void clear();

private:
    using TextureMap = std::unordered_map<ResourceLocation, Texture>;
    TextureMap m_textures;
    SDL_Renderer* m_renderer = nullptr;
    std::unique_ptr<ImageLoader> m_image_loader;
};
} // namespace serenkai