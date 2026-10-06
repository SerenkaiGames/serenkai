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

protected:
    bool handle_mouse_move_event(const MouseMoveEvent& e) override;
    bool handle_key_event(const KeyEvent& e) override;

private:
    std::unique_ptr<Widget> m_root_widget;
};
} // namespace serenkai