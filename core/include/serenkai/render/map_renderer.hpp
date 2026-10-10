#pragma once

#include <SDL3/SDL_render.h>
#include <glm/ext/vector_float2.hpp>
#include <glm/vec2.hpp>
#include <span>
namespace serenkai {

class Map;
class TextureManager;
struct Tile;
struct TileLayer;
struct ImageLayer;

/// @brief Renderer for the map.
class MapRenderer {
public:
    MapRenderer(const MapRenderer&) = delete;
    MapRenderer(MapRenderer&&) = delete;
    MapRenderer& operator=(const MapRenderer&) = delete;
    MapRenderer& operator=(MapRenderer&&) = delete;

    MapRenderer(SDL_Renderer* renderer);
    ~MapRenderer() = default;

    /// @param camera Camera position in the world (usually the player's
    /// position).
    /// @param zoom Camera zoom.
    void render(Map* map, TextureManager* texture_manager, glm::vec2 camera,
                float zoom);

private:
    SDL_Renderer* m_renderer = nullptr;

    void render_tile_layer(const TileLayer* layer,
                           std::span<SDL_Texture*> textures, Map* map,
                           glm::vec2 camera, float zoom);

    void render_tile(SDL_Texture* texture, const SDL_FRect* src, SDL_FRect dst,
                     const Tile& tile);

    void render_image_layer(const ImageLayer* layer,
                            TextureManager* texture_manager, glm::vec2 camera,
                            float zoom, glm::vec2 view);
};
} // namespace serenkai