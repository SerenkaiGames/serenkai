#include "serenkai/render/map_renderer.hpp"

#include "serenkai/game/map.hpp"
#include "serenkai/game/map_data.hpp"
#include "serenkai/resource/texture_manager.hpp"

#include <SDL3/SDL_rect.h>
#include <SDL3/SDL_render.h>
#include <algorithm>
#include <glm/ext/vector_float2.hpp>
#include <span>
#include <variant>
#include <vector>
namespace serenkai {

MapRenderer::MapRenderer(SDL_Renderer* renderer) : m_renderer(renderer) {}

void MapRenderer::render(Map* map, TextureManager* texture_manager,
                         glm::vec2 camera, float zoom) {
    if (!m_renderer || !map || !texture_manager) {
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
    if (zoom <= 0.0f) {
        zoom = 1.0f;
    }

    // Viewport size in world units.
    const glm::vec2 view{static_cast<float>(w) / zoom,
                         static_cast<float>(h) / zoom};

    const auto world = static_cast<glm::vec2>(map->pixel_size());
    const glm::vec2 half = view * 0.5f;

    // If the map is smaller than the viewport on an axis, center it on that
    // axis; otherwise, clamp the viewport within the map's world bounds.
    if (world.x < view.x) {
        camera.x = world.x * 0.5f;
    } else {
        camera.x = std::clamp(camera.x, half.x, world.x - half.x);
    }

    if (world.y < view.y) {
        camera.y = world.y * 0.5f;
    } else {
        camera.y = std::clamp(camera.y, half.y, world.y - half.y);
    }

    const glm::vec2 screen_center{static_cast<float>(w) * 0.5f,
                                  static_cast<float>(h) * 0.5f};

    // World space -> screen space.
    // The camera is the observation point and lands at the window center:
    // p_screen = (p_world - camera) * zoom + screen_center
    const glm::vec2 screen_offset{screen_center.x - camera.x * zoom,
                                  screen_center.y - camera.y * zoom};

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
            render_image_layer(image_layer, texture_manager, camera, zoom);
        }
    }
}

void MapRenderer::render_tile_layer(const TileLayer* layer,
                                    std::span<SDL_Texture*> textures, Map* map,
                                    glm::vec2 screen_offset, float zoom) {
    if (!layer) {
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
            if (idx >= textures.size() || !textures[idx]) {
                continue;
            }

            auto rect = data.tilesets[idx].get_rect(tile->local_id);

            // Rectangle of the tile's position in the texture.
            SDL_FRect src_rect{rect.x, rect.y, rect.z, rect.w};

            auto world_pos = map->tile_to_world({x, y});

            // world -> screen
            SDL_FRect dst_rect = {world_pos.x * zoom + screen_offset.x,
                                  world_pos.y * zoom + screen_offset.y,
                                  tile_size.x * zoom, tile_size.y * zoom};

            render_tile(textures[idx], &src_rect, dst_rect, *tile);
        }
    }
}

void MapRenderer::render_tile(SDL_Texture* texture, const SDL_FRect* src,
                              SDL_FRect dst, const Tile& tile) {
    double angle = 0.0;
    SDL_FlipMode sdl_flip = SDL_FLIP_NONE;

    bool d = tile.flip_diagonal();
    bool h = tile.flip_horizontal();
    bool v = tile.flip_vertical();

    if (!d) {
        if (h) {
            sdl_flip =
                static_cast<SDL_FlipMode>(sdl_flip | SDL_FLIP_HORIZONTAL);
        }
        if (v) {
            sdl_flip = static_cast<SDL_FlipMode>(sdl_flip | SDL_FLIP_VERTICAL);
        }
    } else {
        // Diagonal flip swaps width and height while keeping the tile center
        // invariant.
        float cx = dst.x + dst.w * 0.5f;
        float cy = dst.y + dst.h * 0.5f;
        std::swap(dst.w, dst.h);
        dst.x = cx - dst.w * 0.5f;
        dst.y = cy - dst.h * 0.5f;

        if (!h && !v) {
            angle = 90.0;
            sdl_flip = SDL_FLIP_VERTICAL;
        } else if (h && !v) {
            // Most common case in Tiled: 90 degrees clockwise rotation.
            angle = 90.0;
            sdl_flip = SDL_FLIP_NONE;
        } else if (!h && v) {
            // 270 degrees clockwise (90 degrees counter-clockwise).
            angle = 270.0;
            sdl_flip = SDL_FLIP_NONE;
        } else {
            // h && v: 90 degrees + horizontal flip (anti-diagonal reflection).
            angle = 90.0;
            sdl_flip = SDL_FLIP_HORIZONTAL;
        }
    }

    SDL_RenderTextureRotated(m_renderer, texture, src, &dst, angle, nullptr,
                             sdl_flip);
}

void MapRenderer::render_image_layer(const ImageLayer* layer,
                                     TextureManager* texture_manager,
                                     glm::vec2 screen_offset, float zoom) {
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
    auto size = static_cast<glm::vec2>(layer->size);

    // world -> screen
    SDL_FRect dst{screen_offset.x, screen_offset.y, size.x * zoom,
                  size.y * zoom};

    SDL_RenderTexture(m_renderer, texture, nullptr, &dst);
}

} // namespace serenkai
