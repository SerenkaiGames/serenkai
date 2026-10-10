#include "serenkai/scenes/game_scene.hpp"

#include "serenkai/game/camera.hpp"
#include "serenkai/game/map_manager.hpp"
#include "serenkai/render/renderer.hpp"

#include <memory>
#include <stdexcept>

namespace serenkai {

void GameScene::on_enter(AppContext* ctx) {
    if (!ctx) {
        throw std::runtime_error(
            "Failed to enter game scene: Application context is null");
    }
    m_map_manager = std::make_unique<MapManager>(ctx->assets);
    m_camera = std::make_unique<Camera>();
}

void GameScene::update(float dt) {
    if (m_map_manager) {
        m_map_manager->update(dt);
    }
}

void GameScene::render(RenderContext* ctx) {
    if (!ctx) {
        return;
    }
    if (m_map_manager && m_camera) {
        auto map = m_map_manager->current_map();
        if (map) {
            ctx->renderer->draw_map(map.get(), ctx->textures, m_camera->pos(),
                                    m_camera->zoom());
        }
    }
}
} // namespace serenkai