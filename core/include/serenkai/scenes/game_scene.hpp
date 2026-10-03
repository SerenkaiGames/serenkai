#pragma once

#include "serenkai/scenes/scene.hpp"
namespace serenkai {
class GameScene : public Scene {
public:
    void update(float dt) override;

    void render(GuiContext* context) override;
};
} // namespace serenkai