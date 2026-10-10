#include "serenkai/scenes/title_scene.hpp"

#include "serenkai/application/app_context.hpp"
#include "serenkai/gui/gui_context.hpp"
#include "serenkai/gui/widget_parser.hpp"
#include "serenkai/render/render_context.hpp"

#include <stdexcept>
namespace serenkai {

void TitleScene::update(float dt) {
    if (m_root_widget) {
        m_root_widget->update(dt);
    }
}

void TitleScene::render(RenderContext* ctx) {
    if (m_root_widget) {
        m_root_widget->render(ctx->gui);
    }
}

void TitleScene::on_enter(AppContext* ctx) {
    if (!ctx) {
        throw std::runtime_error(
            "Failed enter to tille scene, the app context is nullptr");
    }
    m_root_widget =
        ctx->gui->get_widget_parser()->parse("serenkai:ui/main_title.json");
}

bool TitleScene::handle_mouse_move_event(const MouseMoveEvent& e) {
    if (m_root_widget) {
        return m_root_widget->handle_mouse_move_event(e);
    }
    return false;
}

bool TitleScene::handle_key_event(const KeyEvent& e) {
    if (m_root_widget) {
        return m_root_widget->handle_key_event(e);
    }
    return false;
}
} // namespace serenkai