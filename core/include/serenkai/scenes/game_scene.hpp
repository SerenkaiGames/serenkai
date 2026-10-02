#pragma once

#include "serenkai/scenes/scene.hpp"
namespace serenkai {
class GameScene : public Scene {
public:
    void update(float dt) override;

    void render(Renderer& renderer) override;
};
} // namespace serenkai