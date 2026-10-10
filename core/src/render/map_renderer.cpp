#include "serenkai/render/map_renderer.hpp"

#include "serenkai/game/map.hpp"
#include "serenkai/game/map_data.hpp"
#include "serenkai/resource/texture_manager.hpp"

#include <SDL3/SDL_rect.h>
#include <SDL3/SDL_render.h>
#include <glm/ext/vector_float2.hpp>
#include <span>
#include <variant>
#include <vector>
namespace serenkai {

MapRenderer::MapRenderer(SDL_Renderer* renderer) : m_renderer(renderer) {}

void MapRenderer::render(Map* map, TextureManager* texture_manager,
                         glm::vec2 camera, float zoom) {
    if (!m_renderer) {
        return;
    }

    int w = 0, h = 0;
    SDL_GetRenderOutputSize(m_renderer, &w, &h);

    /*
        The viewport should be set by the external rendering pipeline.
        Here, it must be ensured that the viewport size matches the screen size.
        SDL_Rect vp = {0, 0, w, h};
        SDL_SetRenderViewport(m_renderer, &vp);
    */

    float view_w = w / zoom;
    float view_h = h / zoom;

    if (camera.x < 0) {
        camera.x = 0;
    }

    if (camera.y < 0) {
        camera.y = 0;
    }

    auto world_size = map->pixel_size();

    // Clamp the camera position so it does not go beyond the screen edges.
    if (camera.x > world_size.x - view_w) {
        camera.x = world_size.x - view_w;
    }

    if (camera.y > world_size.y - view_h) {
        camera.y = world_size.y - view_h;
    }

    auto& data = map->data();

    std::vector<SDL_Texture*> textures;

    // Collect all tileset textures in advance to avoid a large number of
    // repeated hash lookups.
    for (auto& set : data.tilesets) {
        auto* texture = texture_manager->get(set.loc);
        textures.emplace_back(texture);
    }

    for (auto& layer : data.layers) {
        if (auto* tile_layer = std::get_if<TileLayer>(&layer)) {
            render_tile_layer(tile_layer, textures, map, camera, zoom);
        }
        if (auto* image_layer = std::get_if<ImageLayer>(&layer)) {
            render_image_layer(image_layer, texture_manager, camera, zoom,
                               glm::vec2{view_w, view_h});
        }
    }
}

void MapRenderer::render_tile_layer(const TileLayer* layer,
                                    std::span<SDL_Texture*> textures, Map* map,
                                    glm::vec2 camera, float zoom) {
    if (layer) {
        return;
    }
    if (!layer->visible) {
        return;
    }

    auto layer_size = layer->size;
    auto& data = map->data();
    auto tile_size = map->tile_size();

    for (int x = 0; x < layer_size.x; ++x) {
        for (int y = 0; y < layer_size.y; ++y) {
            auto* tile = layer->get_tile(x, y);
            if (!tile) {
                continue;
            }
            if (tile->is_empty()) {
                continue;
            }
            auto idx = tile->tileset;

            if (!textures[idx]) {
                continue;
            }

            auto rect = data.tilesets[idx].get_rect(tile->local_id);

            // Rectangle of the tile's position in the texture.
            SDL_FRect src_rect{rect.x, rect.y, rect.z, rect.w};

            auto world_pos = map->tile_to_world({x, y});

            // Rectangle of the render position; converts world-view coordinates
            // into camera-view coordinates for rendering.
            SDL_FRect dst_rect = {(world_pos.x - camera.x) * zoom,
                                  (world_pos.y - camera.y) * zoom,
                                  tile_size.x * zoom, tile_size.y * zoom};

            render_tile(textures[idx], &src_rect, dst_rect, *tile);
        }
    }
}

void MapRenderer::render_tile(SDL_Texture* texture, const SDL_FRect* src,
                              SDL_FRect dst, const Tile& tile) {
    double angle = 0.0;
    SDL_FlipMode sdl_flip = SDL_FLIP_NONE;

    if (tile.flip_diagonal()) {
        angle = 90.0;
        sdl_flip = SDL_FLIP_HORIZONTAL;

        float cx = dst.x + dst.w * 0.5f;
        float cy = dst.y + dst.h * 0.5f;
        std::swap(dst.w, dst.h);
        dst.x = cx - dst.w * 0.5f;
        dst.y = cy - dst.h * 0.5f;
    }

    if (tile.flip_horizontal()) {
        sdl_flip = static_cast<SDL_FlipMode>(sdl_flip | SDL_FLIP_HORIZONTAL);
    }

    if (tile.flip_vertical()) {
        sdl_flip = static_cast<SDL_FlipMode>(sdl_flip | SDL_FLIP_VERTICAL);
    }

    SDL_RenderTextureRotated(m_renderer, texture, src, &dst, angle, nullptr,
                             sdl_flip);
}

void MapRenderer::render_image_layer(const ImageLayer* layer,
                                     TextureManager* texture_manager,
                                     glm::vec2 camera, float zoom,
                                     glm::vec2 view) {
    if (!layer || !texture_manager) {
        return;
    }

    if (!layer->visible) {
        return;
    }

    auto* texture = texture_manager->get(layer->loc);

    if (!texture) {
        return;
    }

    // Get the required texture rectangle based on the camera view.
    SDL_FRect dst{-camera.x * zoom, -camera.y * zoom, view.x, view.y};

    SDL_RenderTexture(m_renderer, texture, nullptr, &dst);
}

} // namespace serenkai
