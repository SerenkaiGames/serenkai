#pragma once

#include "serenkai/scenes/scene.hpp"
namespace serenkai {
class GameScene : public Scene {
public:
    bool handle_event(const Event&) override;

    void update(float dt) override;

    void render(Renderer& renderer) override;
};
} // namespace serenkai