#pragma once
#include "serenkai/gui/widget.hpp"
#include "serenkai/scenes/scene.hpp"

#include <memory>
namespace serenkai {
class TitleScene : public Scene {
public:
    void update(float dt) override;

    void render(GuiContext* context) override;

    void on_enter(GuiContext* context) override;

private:
    std::unique_ptr<Widget> m_root_widget;
};
} // namespace serenkai