#pragma once
#include "serenkai/application/app_context.hpp"
#include "serenkai/gui/ui_view.hpp"
#include "serenkai/scenes/scene.hpp"

#include <memory>
namespace serenkai {
class TitleScene : public Scene {
public:
    void update(float dt) override;

    void render(RenderContext* ctx) override;

    void on_enter(AppContext* ctx) override;

    bool handle_event(const Event& e) override;

private:
    std::unique_ptr<UiView> m_ui_view;
};
} // namespace serenkai
