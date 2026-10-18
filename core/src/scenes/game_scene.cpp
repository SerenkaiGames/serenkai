#include "serenkai/scenes/game_scene.hpp"

#include "serenkai/game/camera.hpp"
#include "serenkai/game/entity.hpp"
#include "serenkai/game/entity_manager.hpp"
#include "serenkai/game/map_manager.hpp"
#include "serenkai/game/world_context.hpp"
#include "serenkai/render/renderer.hpp"
#include "serenkai/script/script.hpp"
#include "serenkai/script/script_constants.hpp"
#include "serenkai/script/script_engine.hpp"

#include <algorithm>
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_float3.hpp>
#include <memory>
#include <optional>
#include <spdlog/spdlog.h>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace serenkai {

void GameScene::on_enter(AppContext* ctx) {
    if (!ctx) {
        throw std::runtime_error(
            "Failed to enter game scene: Application context is null");
    }
    m_map_manager = std::make_unique<MapManager>(ctx->assets);
    m_camera = std::make_unique<Camera>();
    m_entity_manager =
        std::make_unique<EntityManager>(ctx->assets, ctx->script);

    if (ctx->script) {

        // Enter the world and inject the related context.
        ctx->script->set_active_world(WorldContext{
            m_camera.get(), m_map_manager.get(), m_entity_manager.get()});

        auto s = ctx->script->create(GAME_SCENE_SCRIPT_LOC);
        if (s) {
            m_script = std::make_unique<Script>(std::move(*s));
            m_script->call<>("on_enter");
        }
    }
}

void GameScene::update(float dt) {
    if (m_map_manager) {
        m_map_manager->update(dt);
    }
    if (m_entity_manager) {
        m_entity_manager->update(dt);
    }
    if (m_script) {
        m_script->call<>("on_update", dt);
    }
}

void GameScene::render(RenderContext* ctx) {
    if (!ctx || !ctx->renderer || !ctx->textures) {
        return;
    }
    if (m_map_manager && m_camera) {
        auto map = m_map_manager->current_map();
        if (map) {
            auto camera = m_camera->pos();
            float zoom = m_camera->zoom();
            auto size = ctx->renderer->render_size();
            // Viewport size in world units.
            const glm::vec2 view{static_cast<float>(size.x) / zoom,
                                 static_cast<float>(size.y) / zoom};

            const auto world = static_cast<glm::vec2>(map->pixel_size());
            const glm::vec2 half = view * 0.5f;

            // If the map is smaller than the viewport on an axis, center it on
            // that axis; otherwise, clamp the viewport within the map's world
            // bounds.
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

            m_camera->set_pos(camera);

            const glm::vec2 screen_center{static_cast<float>(size.x) * 0.5f,
                                          static_cast<float>(size.y) * 0.5f};

            // World space -> screen space.
            // The camera is the observation point and lands at the window
            // center: p_screen = (p_world - camera) * zoom + screen_center
            // offset = screen_center - camera * zoom
            // p_screen = p_world * zoom + offset
            const glm::vec2 screen_offset{screen_center.x - camera.x * zoom,
                                          screen_center.y - camera.y * zoom};

            ctx->renderer->draw_map(map.get(), ctx->textures, screen_offset,
                                    zoom);

            if (m_entity_manager) {
                struct SpriteDraw {
                    glm::vec3 pos{0.0f};

                    std::optional<std::string> texture;

                    const SpriteAnimationData* animation = nullptr;
                    const SpriteFrame* frame = nullptr;
                };
                std::vector<SpriteDraw> pending_sprites;

                m_entity_manager->for_each_alive(
                    [&pending_sprites, this](EntityId id, const Entity& e) {
                        auto* pos =
                            e.get_prop<glm::vec3>("serenkai:props/pos"_rl);
                        if (!pos) {
                            return;
                        }
                        const AnimationPlayer* player =
                            m_entity_manager->animation_player(id);
                        if (player != nullptr && player->frame() != nullptr) {
                            pending_sprites.push_back(
                                SpriteDraw{.pos = *pos,
                                           .texture = std::nullopt,
                                           .animation = player->data(),
                                           .frame = player->frame()});
                            return;
                        }
                        auto* texture = e.get_prop<std::string>(
                            "serenkai:props/texture"_rl);
                        if (!texture) {
                            return;
                        }

                        pending_sprites.push_back(SpriteDraw{
                            .pos = *pos,
                            .texture = *texture,
                        });
                    });

                std::sort(pending_sprites.begin(), pending_sprites.end(),
                          [](const auto& a, const auto& b) {
                              if (a.pos.z != b.pos.z) {
                                  return a.pos.z < b.pos.z;
                              }
                              return a.pos.y < b.pos.y;
                          });

                for (auto& sprite : pending_sprites) {
                    if (sprite.animation != nullptr &&
                        sprite.frame != nullptr) {

                        const glm::vec2 draw_pos{
                            sprite.pos.x + sprite.frame->offset.x,
                            sprite.pos.y + sprite.frame->offset.y};

                        ctx->renderer->draw_sprite_frame(
                            sprite.animation->image, ctx->textures, draw_pos,
                            sprite.frame->src, screen_offset, zoom);
                    }
                    if (sprite.texture) {
                        ctx->renderer->draw_sprite(
                            sprite.texture.value(), ctx->textures,
                            {sprite.pos.x, sprite.pos.y}, screen_offset, zoom);
                    }
                }
            }
        }
    }
}

void GameScene::on_leave(AppContext* ctx) {
    if (m_script) {
        m_script->call<>("on_leave");
    }
    if (ctx && ctx->script) {

        // Clear the corresponding context.
        ctx->script->set_active_world(WorldContext{});
    }
}

} // namespace serenkai
