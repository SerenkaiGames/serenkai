#pragma once
#include "serenkai/scenes/scene.hpp"
namespace serenkai {
class TitleScene : public Scene {
public:
    bool handle_event(const Event& e) override;

    void update(float dt) override;

    void render(Renderer& renderer) override;
};
} // namespace serenkai