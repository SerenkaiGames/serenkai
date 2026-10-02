#pragma once
#include "serenkai/scenes/scene.hpp"
namespace serenkai {
class TitleScene : public Scene {
public:
    void update(float dt) override;

    void render(Renderer& renderer) override;
};
} // namespace serenkai