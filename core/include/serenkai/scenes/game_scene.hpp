#pragma once

#include "serenkai/game/camera.hpp"
#include "serenkai/game/map_manager.hpp"
#include "serenkai/render/render_context.hpp"
#include "serenkai/scenes/scene.hpp"

#include <memory>
namespace serenkai {
class GameScene : public Scene {
public:
    void on_enter(AppContext* ctx) override;

    void update(float dt) override;

    void render(RenderContext* ctx) override;

private:
    std::unique_ptr<MapManager> m_map_manager;
    std::unique_ptr<Camera> m_camera;
};
} // namespace serenkai