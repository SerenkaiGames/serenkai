#pragma once
#include "serenkai/scenes/scene.hpp"
namespace serenkai {
class TitleScene : public Scene {
public:
    void update(float dt) override;

    void render(GuiContext* context) override;
};
} // namespace serenkai