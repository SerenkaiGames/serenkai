#include "serenkai/scenes/game_scene.hpp"

#include "serenkai/game/camera.hpp"
#include "serenkai/game/map_manager.hpp"
#include "serenkai/game/world_context.hpp"
#include "serenkai/render/renderer.hpp"
#include "serenkai/script/script.hpp"
#include "serenkai/script/script_constants.hpp"
#include "serenkai/script/script_engine.hpp"

#include <memory>
#include <spdlog/spdlog.h>
#include <stdexcept>
#include <utility>

namespace serenkai {

void GameScene::on_enter(AppContext* ctx) {
    if (!ctx) {
        throw std::runtime_error(
            "Failed to enter game scene: Application context is null");
    }
    m_map_manager = std::make_unique<MapManager>(ctx->assets);
    m_camera = std::make_unique<Camera>();

    if (ctx->script) {

        // Enter the world and inject the related context.
        ctx->script->set_active_world(WorldContext{
            m_camera.get(), m_map_manager.get(), m_entity_manager.get()});

        auto s = ctx->script->create(GAME_SCENE_SCRIPT_LOC);
        if (s) {
            m_script = std::make_unique<Script>(std::move(*s));
            m_script->call<>("on_enter", m_map_manager.get(), m_camera.get());
        }
    }
}

void GameScene::update(float dt) {
    if (m_map_manager) {
        m_map_manager->update(dt);
    }
    if (m_script) {
        m_script->call<>("on_update", dt);
    }
}

void GameScene::render(RenderContext* ctx) {
    if (!ctx) {
        return;
    }
    if (m_map_manager && m_camera) {
        auto map = m_map_manager->current_map();
        if (map) {
            auto camera_pos = ctx->renderer->draw_map(
                map.get(), ctx->textures, m_camera->pos(), m_camera->zoom());
            m_camera->set_pos(camera_pos);
        }
    }
}

void GameScene::on_leave(AppContext* ctx) {
    if (m_script) {
        m_script->call<>("on_leave");
    }
    // Enter the world and inject the related context.
    if (ctx && ctx->script) {

        // Clear the corresponding context.
        ctx->script->set_active_world(WorldContext{});
    }
}

} // namespace serenkai